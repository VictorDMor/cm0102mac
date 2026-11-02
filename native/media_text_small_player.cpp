// Press texts about a player's words and a manager's answer: apologies and admissions.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

// 0x10649ad0: the player apologises or admits fault. The item holds the player (value 0) and
// his name key (1-3), the club (4), the kind of statement (5) and its tone (7).
void player_apology_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10649ad0, 0x1580, {.first_gpr = 22});
    keep(c, {{30, c.toc(rows_slot)}, {27, item}, {31, c.toc(slot::pool)}, {28, output}});
    const auto name = frame.at(0x38), text = frame.at(0x1c8);
    const Finish finish{c, output, text, name};
    if (compare(c, std::int8_t(full)) != 0) {
        Kept<29> player{c, item_value(c, item, 0, 0x10649b04)};
        Kept<26> first{c, item_value(c, item, 1, 0x10649b18)};
        Kept<25> second{c, item_value(c, item, 2, 0x10649b2c)};
        Kept<24> third{c, item_value(c, item, 3, 0x10649b40)};
        Kept<23> club_index{c, item_value(c, item, 4, 0x10649b54)};
        Kept<22> kind{c, item_value(c, item, 5, 0x10649b68)};
        item_value(c, item, 6, 0x10649b7c);
        Kept<27> tone{c, item_value(c, item, 7, 0x10649b88)};
        club_index = club_record(c, club_index);
        const std::uint32_t club = club_index;
        person_name(c, first, second, third, name, 1, player, 0x10649bbc);
        const Say say{c, text};
        const auto plain = [&](unsigned phrase, std::uint32_t back) { say(phrase, back, {name}); };
        switch (std::int32_t(kind)) {
        case 0: case 2: case 3: case 4: case 5: case 0xd: case 0xe:
            switch (std::int32_t(tone)) {
            case 0: say(0x267, 0x10649ce8, {sign8(c.u8(club + club_short_article)), sign8(c.u8(club + club_short_article)), name, club + club_short_name}); break;
            case 1: say(0x330, 0x10649d00, {0x64, name}); break;
            case 2: plain(0x3b1, 0x10649d14); break;
            default: plain(0x46e, 0x10649d28);
            }
            break;
        case 1: plain(0x532, 0x10649d3c); break;
        case 6: plain(0x7be, 0x10649d8c); break;
        case 8: plain(0x95e, 0x10649db4); break;
        case 0xa: plain(0xab2, 0x10649ddc); break;
        case 0x11: plain(0x180, 0x10649ca0); break;
        case 0x13: plain(0x9e9, 0x10649dc8); break;
        case 0x1e: plain(0x5ea, 0x10649d50); break;
        case 0x20: say(0x6c9, 0x10649d78, {sign8(c.u8(club + club_article)), sign8(c.u8(club + club_article)), name, club + club_name}); break;
        case 0x23: plain(0x874, 0x10649da0); break;
        case 0x25: plain(0x27, 0x10649c78); break;
        case 0x78: plain(0xbd, 0x10649c8c); break;
        default: plain(0xb4b, 0x10649df0);
        }
        finish(player, first, second, third, 0x10649dfc, 0x10649e1c, 0x10649e40);
    } else {
        Kept<22> player{c, item_value(c, item, 0, 0x10649e50)};
        Kept<23> first{c, item_value(c, item, 1, 0x10649e64)};
        Kept<24> second{c, item_value(c, item, 2, 0x10649e78)};
        Kept<25> third{c, item_value(c, item, 3, 0x10649e8c)};
        item_value(c, item, 4, 0x10649ea0);
        Kept<26> kind{c, item_value(c, item, 5, 0x10649eac)};
        item_value(c, item, 6, 0x10649ec0);
        Kept<27> tone{c, item_value(c, item, 7, 0x10649ecc)};
        person_name(c, first, second, third, name, 5, player, 0x10649ef4);
        if (compare(c, kind, 0x25) == 0) format(c, text, 0xc0a, 0x10649f0c, {name});
        else if (compare(c, std::int8_t(tone & 1)) != 0) format(c, text, 0xc62, 0x10649f34, {name});  // tone odd
        else format(c, text, 0xc8e, 0x10649f48, {name});
        finish(player, first, second, third, 0x10649f54, 0x10649f74, 0x10649f98);
    }
}

// 0x10655030: a player answers the manager's words in the press. The item holds the player (value 0)
// with his name key (1-3), the club (4), the kind of exchange (5), its flag (6) and tone (7), a second
// person (8) with a key (9-11) and, for the long form, an assistant (12-16).
void manager_exchange_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x10655030, 0x870, {.first_gpr = 13});
    keep(c, {{29, c.toc(slot::text)}, {14, item}, {30, c.toc(rows_slot)}, {15, output}, {31, c.toc(slot::pool)}});
    const auto text = c.r(29);
    // The player's name in full and short form, the second person's in full and short form, the assistant's.
    const auto subject = frame.at(0x680), subject_short = frame.at(0x4f0), other = frame.at(0x360), other_short = frame.at(0x1d0),
               assistant = frame.at(0x40);
    const Say say{c, text};
    const auto link_person = [&](std::uint32_t span, std::uint32_t person, std::uint32_t back) {
        link_text(c, output, span, 0x7d7, person_row(c, person), back);
    };
    if (compare(c, std::int8_t(full)) != 0) {
        Kept<28> player{c, item_value(c, item, 0, 0x10655068)};
        Kept<27> first{c, item_value(c, item, 1, 0x1065507c)};
        Kept<26> second{c, item_value(c, item, 2, 0x10655090)};
        Kept<25> third{c, item_value(c, item, 3, 0x106550a4)};
        c.put32(frame.at(0x810), item_value(c, item, 4, 0x106550b8));
        Kept<24> kind{c, item_value(c, item, 5, 0x106550cc)};
        Kept<23> flag{c, item_value(c, item, 6, 0x106550e0)};
        Kept<22> tone{c, item_value(c, item, 7, 0x106550f4)};
        Kept<21> other_player{c, item_value(c, item, 8, 0x10655108)};
        Kept<20> other_first{c, item_value(c, item, 9, 0x1065511c)};
        Kept<19> other_second{c, item_value(c, item, 10, 0x10655130)};
        Kept<18> other_third{c, item_value(c, item, 11, 0x10655144)};
        // Value 13 (the assistant's index) is read only when value 12 is 0x420e9; otherwise -1.
        std::uint32_t assistant_index = 0xffffffffu;
        if (item_value(c, item, 12, 0x10655158) == 0x420e9) assistant_index = item_value(c, item, 13, 0x1065518c);
        Kept<17> assistant_player{c, assistant_index};
        Kept<13> assistant_first{c, item_value(c, item, 14, 0x106551a0)};
        // r14 held the item until here.
        const auto assistant_second = item_value(c, item, 15, 0x106551b4);
        Kept<14> assistant_second_kept{c, assistant_second};
        Kept<16> assistant_third{c, item_value(c, item, 16, 0x106551c8)};
        const auto defaults = c.toc(-0x4210);
        for (std::uint32_t at = 0; at < 0x190; at += 4) c.put32(assistant + at, c.u32(defaults + at));
        person_name(c, other_first, other_second, other_third, other, 1, other_player, 0x10655214);
        person_name(c, other_first, other_second, other_third, other_short, 5, other_player, 0x10655234);
        if (compare(c, assistant_player, -1) > 0)
            person_name(c, assistant_first, assistant_second_kept, assistant_third, assistant, 1, assistant_player, 0x1065525c);
        Kept<13> club_kept{c, club_record(c, c.u32(frame.at(0x810)))};
        const std::uint32_t club = club_kept;
        person_name(c, first, second, third, subject, 1, player, 0x10655290);
        person_name(c, first, second, third, subject_short, 5, player, 0x106552b0);
        const auto s8 = sign8(c.u8(club + club_article));
        switch (std::int32_t(kind)) {
        case 0x82: say(0xc57a, 0x106553a8, {subject}); break;
        case 0x13: say(0xc67c, 0x106553c0, {subject}); break;
        case 5: case 7:
            switch (std::int32_t(tone)) {
            case 0: say(0xc731, 0x106553fc, {subject, other}); break;
            case 1: say(0xc83b, 0x1065542c, {s8, s8, club + club_name, subject, other}); break;
            case 2: say(0xc901, 0x1065545c, {s8, s8, subject, club + club_name, other}); break;
            default: say(0xca17, 0x10655490, {s8, s8, 0x64, club + club_name, subject, other});
            }
            break;
        case 9:
            if (std::int32_t(tone) == 0) say(0xcaef, 0x106554c4, {subject});
            else if (std::int32_t(tone) == 1) say(0xcb89, 0x106554dc, {subject});
            else say(0xcc5d, 0x106554f4, {subject});
            break;
        case 2:
            if (std::int32_t(tone) == 0) say(0xcd28, 0x10655540, {0x64, s8, s8, subject, other, club + club_name});
            else say(0xce27, 0x1065556c, {s8, s8, subject, club + club_name});
            break;
        case 4:
            if (compare(c, flag, 1) == 0) say(0xcec8, 0x1065558c, {subject});
            else if (std::int32_t(tone) == 0) say(0xcf86, 0x106555d0, {0x64, 0x64, other, subject, other_short});
            else if (std::int32_t(tone) == 1) say(0xd099, 0x106555ec, {subject, other});
            else say(0xd154, 0x10655610, {0x64, subject, other, other_short});
            break;
        case 0x23: say(0xd270, 0x10655628, {subject}); break;
        case 0: case 6: case 0xd: case 0xe: case 0x11:
            if (compare(c, flag, 1) == 0) { say(0xd32f, 0x10655648, {subject}); break; }
            switch (std::int32_t(tone)) {
            case 0:
                if (compare(c, assistant_player, -1) > 0) say(0xd3ec, 0x10655698, {assistant, subject});
                else say(0xd4ca, 0x106556b0, {subject});
                break;
            case 1:
                if (compare(c, assistant_player, -1) > 0) say(0xd593, 0x106556d4, {assistant, subject});
                else say(0xd668, 0x106556ec, {subject});
                break;
            case 2: say(0xd727, 0x10655708, {0x64, subject}); break;
            case 3: say(0xd7e4, 0x10655728, {0x64, subject, other}); break;
            default: say(0xd8af, 0x10655740, {subject});
            }
            break;
        case 0xc: case 0x22: say(0xd96e, 0x10655758, {subject}); break;
        case 0x21:
            if (std::int32_t(tone) == 0) say(0xd9dd, 0x10655794, {0x64, subject, other});
            else if (std::int32_t(tone) == 1) say(0xda6c, 0x106557b4, {0x64, other, subject});
            else say(0xd96e, 0x106557cc, {subject});
            break;
        case 0x12: say(0xd96e, 0x106557e4, {subject}); break;
        case 3: say(0xdb79, 0x106557fc, {subject}); break;
        case 8: say(0xdc4b, 0x10655814, {subject}); break;
        case 1: say(0xdc4b, 0x1065582c, {subject}); break;
        case 0x1f: say(0xdc4b, 0x10655844, {subject}); break;
        case 0x25: say(0xdcc6, 0x1065585c, {subject}); break;
        case 0x1e:
            if (std::int32_t(tone) == 0) {
                say(0xdd62, 0x106558b4, {0x64, s8, s8, other, subject, club + club_name, subject_short});
            } else if (std::int32_t(tone) == 1) say(0xdec1, 0x106558d4, {subject, other, subject_short});
            else say(0xdc4b, 0x106558ec, {subject});
            break;
        case 0x20: say(0xdc4b, 0x10655904, {subject}); break;
        case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: case 0x30:
        case 0x31: case 0x32: case 0x33: case 0x34: say(0xdfd7, 0x1065591c, {subject}); break;
        default: say(0xdc4b, 0x10655934, {subject});
        }
        copy_text(c, output, text, 0x10655940);
        if (person_listed(c, person_row(c, player), first, second, third, 0x10655960) != 0) {
            link_person(subject, player, 0x10655984);
            link_person(subject_short, player, 0x106559a0);
        }
        if (person_listed(c, person_row(c, other_player), other_first, other_second, other_third, 0x106559c0) != 0) {
            link_person(other, other_player, 0x106559e4);
            link_person(other_short, other_player, 0x10655a00);
        }
        if (compare_logical(c, club) != 0) {
            link_text(c, output, club + club_name, 0x7d5, club, 0x10655a1c);
            link_text(c, output, club + club_short_name, 0x7d5, club, 0x10655a30);
        }
    } else {
        Kept<13> player{c, item_value(c, item, 0, 0x10655a40)};
        Kept<16> first{c, item_value(c, item, 1, 0x10655a54)};
        Kept<17> second{c, item_value(c, item, 2, 0x10655a68)};
        Kept<18> third{c, item_value(c, item, 3, 0x10655a7c)};
        item_value(c, item, 4, 0x10655a90);
        Kept<19> kind{c, item_value(c, item, 5, 0x10655a9c)};
        Kept<14> flag{c, item_value(c, item, 6, 0x10655ab0)};
        Kept<20> tone{c, item_value(c, item, 7, 0x10655ac4)};
        person_name(c, first, second, third, subject, 5, player, 0x10655aec);
        const auto thanks = [&](std::uint32_t back) { say(0xe0ca, back, {subject}); };
        switch (std::int32_t(kind)) {
        case 0x13: say(0xe038, 0x10655bc0, {subject}); break;
        case 0: case 6: case 0xd: case 0xe: case 0x11: case 0x23:
            if (compare(c, flag, 1) == 0) say(0xe074, 0x10655be0, {subject});
            else if (compare(c, tone) == 0 || compare(c, tone, 1) == 0) say(0xe0ac, 0x10655c04, {});
            else thanks(0x10655c1c);
            break;
        case 2: case 4: case 5: case 7: case 9: case 0x82: thanks(0x10655c34); break;
        case 0x12: case 0x21: case 0x22: say(0xe0ff, 0x10655c4c, {subject}); break;
        case 3: thanks(0x10655c64); break;
        case 8: thanks(0x10655c7c); break;
        case 1: thanks(0x10655c94); break;
        case 0x1f: thanks(0x10655cac); break;
        case 0x25: thanks(0x10655cc4); break;
        case 0x1e: case 0x20: thanks(0x10655cdc); break;
        default: thanks(0x10655cf4);
        }
        copy_text(c, output, text, 0x10655d00);
        if (person_listed(c, person_row(c, player), first, second, third, 0x10655d20) != 0)
            link_person(subject, player, 0x10655d44);
    }
}
}
