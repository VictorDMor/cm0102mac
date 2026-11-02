// A transfer story in the press: a player (a staff member by three name parts) is linked with one
// club or with a list of clubs, and the user may be asked about it. Four story types (the
// player eyed, rumoured, linked and chased) share the structure: a paragraph for the lead club,
// a second one when more clubs are interested, the user's reply, and the links.
#include "media_text_mid.hpp"
#include "media_text_mid_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::text;

namespace {
constexpr std::uint32_t amount_text = 0x100d6300, amount_with_staff = 0x100d6af0, region_link = 0x100c76c0;
constexpr int story_eyed = 0x24, story_rumour = 0;

// The list of clubs the story names, in the frame: up to 50 words (the record's own limit).
struct ClubList {
    Cpu& c;
    std::uint32_t at;
    std::uint32_t id(unsigned index) const { return c.u32(at + 4 * index); }
    Club club(unsigned index) const { return club_at(c, id(index)); }
};

// The paragraph about the other clubs interested, for each story type: the phrase and the return address of its
// format call (the append that follows returns 0xc later), by the number of clubs (2-5).
struct Paragraph { std::uint32_t phrase, back; };
constexpr Paragraph paragraphs[4][4]{
    {{0x3b9d1, 0x1069b3a8}, {0x3ba61, 0x1069b470}, {0x3bb07, 0x1069b570}, {0x3bbc3, 0x1069b6c0}},  // eyed
    {{0x3be52, 0x1069b8dc}, {0x3beac, 0x1069b98c}, {0x3bf29, 0x1069ba8c}, {0x3bfc5, 0x1069bbdc}},  // rumour
    {{0x3c4f5, 0x1069c070}, {0x3c561, 0x1069c120}, {0x3c5f0, 0x1069c220}, {0x3c69e, 0x1069c370}},  // linked
    {{0x3c97d, 0x1069c5a4}, {0x3caa2, 0x1069c6c8}, {0x3cb2e, 0x1069c7c8}, {0x3cbd9, 0x1069c918}},  // chased
};
}

void transfer_rumour_text(Cpu& c, std::uint32_t record, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1069ace0, 0x5c0, {.first_gpr = 13});
    keep(c, {{15, record}, {29, c.toc(slot::text)}, {30, c.toc(slot::clubs)}, {16, output}, {13, c.toc(staff_slot)}, {31, c.toc(slot::pool)}, {26, 0}});
    Kept<28> cursor(c, 0);
    c.put8(frame.at(0x5e3), full);
    Program program{c};
    // The record, field after field; the register that counts them is a step ahead of the field being read.
    RecordReader read{.c = c, .record = record, .cursor = 28};
    Kept<25> staff_id(c, read.counted(0x1069ad24));
    Kept<14> first(c, read.counted(0x1069ad38));
    Kept<24> middle(c, read.counted(0x1069ad4c));
    Kept<23> last(c, read.counted(0x1069ad60));
    Kept<22> other_id(c, read.counted(0x1069ad74));
    Kept<21> story(c, read.counted(0x1069ad88));
    Kept<20> money(c, read.counted(0x1069ad9c));
    Kept<19> reply(c, read.counted(0x1069adb0));
    const auto amount_id = frame.at(0x55c), amount_a = frame.at(0x560), second_first = frame.at(0x558), second_middle = frame.at(0x554), second_last = frame.at(0x550),
               undecided = frame.at(0x54c), list = frame.at(0x54);
    c.put32(amount_a, read.counted(0x1069adc4));
    c.put32(amount_id, read.counted(0x1069add8));
    Kept<18> second_staff(c, read.counted(0x1069adec));
    c.put32(second_first, read.counted(0x1069ae00));
    c.put32(second_middle, read.counted(0x1069ae14));
    c.put32(second_last, read.counted(0x1069ae28));
    c.put8(undecided, signed_byte(read.counted(0x1069ae3c)));
    Kept<17> count(c, read.counted(0x1069ae58));
    Kept<27> index(c, 0);
    // The clubs: a count of them, read until the record's fifty fields run out.
    while (compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(std::uint32_t(count))) < 0) {
        if (compare(c, signed_byte(cursor), 0x32) == 0) {
            count = 0;
            break;
        }
        const auto club = read.counted(0x1069ae88);
        c.put32(list + 4 * std::uint32_t(index), club);
        index = std::uint32_t(index) + 1;
    }
    const auto amount_text_at = frame.at(0x2f4);
    Kept<15> money_text(c, program.call(amount_text, 0x1069aeb8, {extended(std::int16_t(c.u32(amount_id))), amount_text_at, 0xc8, 1}));
    money_text = program.call(amount_with_staff, 0x1069aee4,
                              {staff_id, extended(std::int16_t(c.u32(amount_id))), c.u32(amount_a), amount_text_at, 0xc8, money_text, 1});
    // A region record the story's text may name; the sum of the record's figures mod 6 decides which.
    Kept<26> region(c, c.r(26));
    const ClubList clubs{c, list};
    if (compare(c, std::int32_t(std::uint32_t(other_id)), -1) > 0) {
        const auto sum = std::int32_t(std::uint32_t(count) + std::uint32_t(staff_id) + std::uint32_t(other_id) + std::uint32_t(story) + std::uint32_t(second_staff));
        const auto rest = sum % 6;
        const auto staff_region = [&] { return c.u32(c.u32(c.r(13)) + std::uint32_t(staff_id) * staff_size + 0xc); };
        // The staff member's own region, when his record's rating (+0xac) is 6000 or more.
        const auto from_staff = [&] {
            if (compare(c, std::int32_t(c.i16(staff_region() + 0xac)), 0x1770) >= 0) region = staff_region();
        };
        int which = 0;
        if (const auto against_three = compare(c, rest, 3); against_three == 0) which = 3;
        else if (against_three > 0) which = compare(c, rest, 6) >= 0 ? 0 : 4;
        else if (compare(c, rest, 1) >= 0) which = 1;
        if (which == 1) {
            // The original tests for the absence of another club once more here, and the test cannot succeed inside this block (its else is dead).
            compare(c, std::int32_t(std::uint32_t(other_id)), -1);
            region = c.u32(club_at(c, other_id).at + 0x2c);
        } else if (which == 3) from_staff();
        else if (which == 4) region = c.u32(clubs.club(0).at + 0x2c);
    }
    // Whether the story's club and the user's club are of one nation: the user's manager record's club has the region (or he has
    // none, and the region is the manager's own), or there is no region to speak of.
    Kept<27> shares(c, 0);
    {
        const auto base = c.toc(user_slot);
        const auto manager = c.u32(base + std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0 + 4);
        if (manager != 0) {
            if (region == 0) shares = 1;
            else if (const auto manager_club = c.u32(manager + 0x24); manager_club != 0) shares = c.u32(manager_club + 0x2c) == std::uint32_t(region) ? 1 : 0;
            else shares = std::uint32_t(region) == c.u32(manager + 0xc) ? 1 : 0;
        }
    }
    const auto clubs_base = table(c, -0x7be8);
    const Output out{c, output};
    const auto text = c.toc(slot::text);
    const auto name = frame.at(0x27c), short_name = frame.at(0x204), second_name = frame.at(0x18c), second_short = frame.at(0x114), amount_buffer = frame.at(0x68),
               nation_name = frame.at(0x9c), reply_text = frame.at(0x3bc);
    const auto sb_money = [&] { return signed_byte(money_text); };
    const auto other = [&] { return club_at(c, other_id); };
    const auto user_manager = [&] {
        const auto base = c.toc(user_slot);
        return c.u32(base + std::uint32_t(std::int32_t(c.i16(base + 0x2c58))) * 0x2c0 + 4);
    };
    const Club lead = clubs.club(0);
    const auto sb_lead_short = [&] { return signed_byte(c.u8(lead.at + 0x23b)); };
    if (compare(c, signed_byte(c.u8(frame.at(0x5e3)))) == 0) {
        program.call(entry::person_name, 0x1069ce90, {first, middle, last, name, 0x190, 5, staff_id});
        if (compare(c, std::int32_t(std::uint32_t(story)), story_eyed) == 0) {
            if (compare(c, std::int32_t(std::uint32_t(other_id)), -1) == 0 && compare(c, c.i32(list), -1) > 0) {
                format(c, text, 0x3d08b, 0x1069cf04, {sb_lead_short(), sb_lead_short(), name, lead.short_name(), clubs_base, clubs_base});
            } else {
                const auto id = std::int32_t(std::uint32_t(staff_id));
                const bool odd = compare(c, ((id & 1) ^ std::int32_t(std::uint32_t(id) >> 31)) - std::int32_t(std::uint32_t(id) >> 31)) != 0;
                if (odd) format(c, text, 0x3d0d4, 0x1069cf30, {name});
                else format(c, text, 0x3d10f, 0x1069cf48, {name});
            }
        } else {
            const auto rest = std::int32_t(std::uint32_t(staff_id)) % 3;
            const auto against_one = compare(c, rest, 1);
            if (against_one < 0 && compare(c, rest, 0) >= 0) format(c, text, 0x3d140, 0x1069cf94, {name});
            else if (against_one == 0) {
                if (compare(c, std::int32_t(std::uint32_t(count)), 1) > 0) format(c, text, 0x3d17c, 0x1069cfb4, {name});
                else format(c, text, 0x3d1ab, 0x1069cfcc, {name});
            } else if (compare(c, std::int32_t(std::uint32_t(count)), 1) > 0) {
                format(c, text, 0x3d1d9, 0x1069d030, {sb_lead_short(), sb_lead_short(), lead.short_name(), name, clubs_base, clubs_base});
            } else if (compare(c, std::int32_t(std::uint32_t(count)), 1) == 0) {
                format(c, text, 0x3d22d, 0x1069d094, {sb_lead_short(), sb_lead_short(), name, lead.short_name(), clubs_base, clubs_base});
            } else format(c, text, 0x3d1ab, 0x1069d0ac, {name});
        }
        out.set(text, 0x1069d0b8);
        if (out.named(staff_at(c, staff_id), first, middle, last, 0x1069d0d8)) out.link(name, link_kind::staff, staff_at(c, staff_id), 0x1069d0fc);
        if (compare(c, std::int32_t(std::uint32_t(other_id)), -1) > 0)
            out.link(other().long_name(), link_kind::club, other().at, 0x1069d12c, clubs_base);
        for (index = 0; compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(std::uint32_t(count))) < 0; index = std::uint32_t(index) + 1)
            out.link(clubs.club(index).long_name(), link_kind::club, clubs.club(index).at, 0x1069d174, clubs_base);
        return;
    }
    // The long form.
    program.call(entry::person_name, 0x1069b0ac, {first, middle, last, name, 0x78, 1, staff_id});
    program.call(entry::person_name, 0x1069b0cc, {first, middle, last, short_name, 0x78, 5, staff_id});
    if (compare(c, std::int32_t(std::uint32_t(second_staff)), -1) > 0) {
        program.call(entry::person_name, 0x1069b0f4, {c.u32(second_first), c.u32(second_middle), c.u32(second_last), second_name, 0x78, 1, second_staff});
        program.call(entry::person_name, 0x1069b114, {c.u32(second_first), c.u32(second_middle), c.u32(second_last), second_short, 0x78, 5, second_staff});
    }
    c.put32(frame.at(0x50), money);
    program.call(entry::to_amount, 0x1069b124, {frame.at(0x50), 6});
    program.call(entry::format_amount, 0x1069b13c, {amount_buffer, 0x32, c.r(3), c.r(4), 6});
    // The story type picks the family of texts: 0 the player eyed, 1 rumoured, 2 linked, 3 chased.
    unsigned family = 3;
    if (const auto against_one = compare(c, std::int32_t(std::uint32_t(story)), 1); against_one == 0) family = 2;
    else if (against_one > 0) family = compare(c, std::int32_t(std::uint32_t(story)), story_eyed) == 0 ? 0 : 3;
    else family = compare(c, std::int32_t(std::uint32_t(story)), story_rumour) >= 0 ? 1 : 3;
    const auto q = other();
    const auto no_other = [&] { return compare(c, std::int32_t(std::uint32_t(other_id)), -1) == 0; };
    const auto no_money = [&] { return compare(c, std::int32_t(std::uint32_t(money))) == 0; };
    const auto money_buffer = amount_text_at;
    switch (family) {
    case 0:
        if (no_other()) format(c, text, 0x3b796, 0x1069b1c4, {0x64, lead.long_marker(), lead.long_marker(), name, lead.long_name(), clubs_base});
        else if (no_money())
            {
                StackWords{c} << q.long_name() << money_buffer << name << lead.long_name();
                format(c, text, 0x3b80a, 0x1069b27c, {q.long_marker(), q.long_marker(), sb_money(), sb_money(), lead.long_marker(), lead.long_marker()});
            }
        else
            {
                StackWords{c} << q.long_name() << money_buffer << name << lead.long_name() << amount_buffer;
                format(c, text, 0x3b8da, 0x1069b334, {q.long_marker(), q.long_marker(), sb_money(), sb_money(), lead.long_marker(), lead.long_marker()});
            }
        break;
    case 1:
        if (no_other()) format(c, text, 0x3bcb2, 0x1069b734, {0x64, lead.long_marker(), lead.long_marker(), name, lead.long_name(), clubs_base});
        else if (no_money()) {
            // The original leaves the clubs table in r15 (the figure above) from here on.
            money_text = clubs_base;
            StackWords{c} << lead.long_name();
            format(c, text, 0x3bd37, 0x1069b7d4, {q.long_marker(), q.long_marker(), lead.long_marker(), lead.long_marker(), q.long_name(), name});
        } else
        {
            StackWords{c} << amount_buffer << lead.long_name();
            format(c, text, 0x3bdbb, 0x1069b874, {q.long_marker(), q.long_marker(), lead.long_marker(), lead.long_marker(), q.long_name(), name});
        }
        break;
    case 2:
        if (no_money() && no_other()) {
            if (compare(c, signed_byte(shares)) != 0) format(c, text, 0x3c085, 0x1069bc5c, {lead.long_marker(), lead.long_marker(), lead.long_name(), name, clubs_base, clubs_base});
            else
            {
                StackWords{c} << name;
                format(c, text, 0x3c10e, 0x1069bcd0,
                                 {signed_byte(c.u8(region + 0x116)), signed_byte(c.u8(region + 0x116)), lead.long_marker(), lead.long_marker(), region + 0xe1, lead.long_name()});
            }
        } else if (no_money()) {
            if (compare(c, signed_byte(shares)) != 0)
            {
                StackWords{c} << name;
                format(c, text, 0x3c1b5, 0x1069bd78, {lead.long_marker(), lead.long_marker(), q.long_marker(), q.long_marker(), lead.long_name(), q.long_name()});
            }
            else
            {
                StackWords{c} << region + 0xe1 << lead.long_name() << q.long_name() << name;
                format(c, text, 0x3c272, 0x1069be30,
                                 {signed_byte(c.u8(region + 0x116)), signed_byte(c.u8(region + 0x116)), lead.long_marker(), lead.long_marker(), q.long_marker(), q.long_marker()});
            }
        } else if (no_other()) {
            format(c, text, 0x3c085, 0x1069be94, {lead.long_marker(), lead.long_marker(), lead.long_name(), name, clubs_base, clubs_base});
        } else if (compare(c, signed_byte(shares)) != 0) {
            // The original's address arithmetic leaves the clubs table in r21 and the other club's offset in r15.
            story = clubs_base;
            money_text = std::uint32_t(other_id) * club_size;
            StackWords{c} << name << amount_buffer;
            format(c, text, 0x3c34d, 0x1069bf3c, {lead.long_marker(), lead.long_marker(), q.long_marker(), q.long_marker(), lead.long_name(), q.long_name()});
        } else {
            program.call(entry::player_name, 0x1069bf54, {region, nation_name, 0x78, 0x1c});
            StackWords{c} << nation_name << lead.long_name() << q.long_name() << name << amount_buffer;
            format(c, text, 0x3c414, 0x1069c008, {0x64, 0x64, lead.long_marker(), lead.long_marker(), q.long_marker(), q.long_marker()});
        }
        break;
    default:
        if (no_other()) format(c, text, 0x3c770, 0x1069c3e4, {0x64, lead.long_marker(), lead.long_marker(), name, lead.long_name(), clubs_base});
        else if (no_money()) {
            StackWords{c} << lead.long_name() << q.long_name();
            format(c, text, 0x3c7fe, 0x1069c48c, {0x64, lead.long_marker(), lead.long_marker(), q.long_marker(), q.long_marker(), name});
        }
        else {
            StackWords{c} << amount_buffer << q.long_name() << lead.long_name();
            format(c, text, 0x3c8cf, 0x1069c534, {0x64, q.long_marker(), q.long_marker(), lead.long_marker(), lead.long_marker(), name});
        }
        break;
    }
    constexpr std::uint32_t assign_backs[]{0x1069b340, 0x1069b880, 0x1069c014, 0x1069c540};
    out.set(text, assign_backs[family]);
    // The other clubs after the lead one.
    for (std::uint32_t wanted = 2; wanted <= 5; ++wanted) {
        if (compare(c, std::int32_t(std::uint32_t(count)), std::int32_t(wanted)) != 0) continue;
        const auto& paragraph = paragraphs[family][wanted - 2];
        const auto one = clubs.club(1), two = clubs.club(2), three = clubs.club(3), four = clubs.club(4);
        auto back = paragraph.back;
        switch (wanted) {
        case 2:
            if (family == 0) format(c, text, paragraph.phrase, back, {one.long_marker(), one.long_marker(), sb_money(), sb_money(), one.long_name(), money_buffer});
            else if (family != 3 || compare(c, signed_byte(shares)) != 0)
                format(c, text, paragraph.phrase, back, {one.long_marker(), one.long_marker(), one.long_name(), clubs_base, clubs_base, clubs_base});
            else {
                program.call(entry::player_name, 0x1069c5bc, {region, nation_name, 0x78, 0x18});
                back = 0x1069c618;
                format(c, text, 0x3c9ff, back, {0x64, one.long_marker(), one.long_marker(), nation_name, one.long_name(), clubs_base});
            }
            break;
        case 3:
            if (family == 0) {
                StackWords{c} << money_buffer << one.long_name() << two.long_name();
                format(c, text, paragraph.phrase, back, {sb_money(), sb_money(), one.long_marker(), one.long_marker(), two.long_marker(), two.long_marker()});
            } else {
                // The original leaves the clubs table in r15 and the third club's offset in r21 here.
                money_text = clubs_base;
                story = c.u32(list + 8) * club_size;
                format(c, text, paragraph.phrase, back, {one.long_marker(), one.long_marker(), two.long_marker(), two.long_marker(), one.long_name(), two.long_name()});
            }
            break;
        case 4:
            StackWords{c} << one.long_name() << two.long_name() << three.long_name();
            format(c, text, paragraph.phrase, back,
                             {one.long_marker(), one.long_marker(), two.long_marker(), two.long_marker(), three.long_marker(), three.long_marker()});
            break;
        default:
            money_text = clubs_base;
            story = clubs_base;
            StackWords{c} << four.long_marker() << four.long_marker() << one.long_name() << two.long_name() << three.long_name() << four.long_name();
            format(c, text, paragraph.phrase, back,
                             {one.long_marker(), one.long_marker(), two.long_marker(), two.long_marker(), three.long_marker(), three.long_marker()});
            break;
        }
        // (The chased story's nationality version of the two-club paragraph shares its append with the plain one.)
        out.append(text, family == 3 && wanted == 2 ? 0x1069c624 : back + 0xc);
        break;
    }
    // What the user is told about it, when it is his own manager the story names.
    const auto second_member = c.u32(c.r(13)) + std::uint32_t(second_staff) * staff_size;
    const auto asked = compare(c, std::int32_t(std::uint32_t(second_staff)), -1) > 0 && compare_logical(c, second_member, user_manager()) == 0;
    if (asked) {
        const auto reply_kind = reply_of(c, std::int32_t(std::uint32_t(reply)));
        const auto answered = [&] { return compare(c, signed_byte(c.u8(undecided))) != 0; };
        if (reply_kind == 1) {
            if (answered()) format(c, reply_text, 0x3ccac, 0x1069c9a0, {0x64, short_name});
            else format(c, reply_text, 0x3cd54, 0x1069c9b8, {short_name});
        } else if (reply_kind == 2) {
            if (answered()) format(c, reply_text, 0x3cdc4, 0x1069c9dc, {short_name});
            else if (compare(c, std::int32_t(std::uint32_t(money))) != 0) format(c, reply_text, 0x3ce9c, 0x1069ca14, {short_name});
            else format(c, reply_text, 0x3ce1e, 0x1069c9fc, {short_name});
        } else if (reply_kind == 3) format(c, reply_text, 0x3cf1a, 0x1069ca28, {});
        else c.put8(reply_text, 0);
        out.append(reply_text, 0x1069ca40);
    } else {
        int which = 0;
        if (const auto against_two = compare(c, std::int32_t(std::uint32_t(reply)), 2); against_two == 0) which = 2;
        else if (against_two < 0 && compare(c, std::int32_t(std::uint32_t(reply)), 1) >= 0) which = 1;
        if (which == 1) format(c, reply_text, 0x3cf4d, 0x1069cab0, {q.short_marker(), q.short_marker(), q.short_name(), second_name, short_name});
        else if (which == 2) format(c, reply_text, 0x3cfe9, 0x1069cb04, {q.short_marker(), q.short_marker(), q.short_name(), second_name, short_name});
        else c.put8(reply_text, 0);
        out.append(reply_text, 0x1069cb1c);
    }
    const auto member = staff_at(c, staff_id);
    if (out.named(member, first, middle, last, 0x1069cb3c)) {
        out.link(name, link_kind::staff, member, 0x1069cb60);
        out.link(short_name, link_kind::staff, member, 0x1069cb7c);
    }
    if (compare(c, std::int32_t(std::uint32_t(second_staff)), -1) > 0)
        out.staff_link(staff_at(c, second_staff), c.u32(second_first), c.u32(second_middle), c.u32(second_last), second_name, 0x1069cba4, 0x1069cbc8);
    if (compare(c, std::int32_t(std::uint32_t(other_id)), -1) > 0) out.link(q.long_name(), link_kind::club, q.at, 0x1069cbf8, clubs_base);
    if (compare_logical(c, region) != 0) {
        const auto target = [&](std::uint32_t back) { return program.call(region_link, back, {region}); };
        out.link(region + 0xae, link_kind::club, target(0x1069cc08), 0x1069cc1c);
        out.link(region + 0xe1, link_kind::club, target(0x1069cc24), 0x1069cc38);
        for (unsigned code = 0; code < 10; ++code) {
            const auto step = 0x30 * code;
            program.call(entry::player_name, 0x1069cc4c + step, {region, nation_name, 0x78, 0x16 + code});
            out.link(nation_name, link_kind::club, target(0x1069cc54 + step), 0x1069cc68 + step);
        }
    }
    for (index = 0; compare(c, std::int32_t(std::uint32_t(index)), std::int32_t(std::uint32_t(count))) < 0; index = std::uint32_t(index) + 1)
        out.link(clubs.club(index).long_name(), link_kind::club, clubs.club(index).at, 0x1069ce60, clubs_base);
}
}
