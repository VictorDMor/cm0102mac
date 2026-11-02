// Press texts about what a player or his agent asks of the manager.
#include "media_text_small.hpp"
#include "media_text_small_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::small;

// 0x1067d2f0: a player, or his agent, makes a request. The item holds the player (value 0) with his name
// key (1-3), the club (4) and the kind of request, its tone and details (5-9).
void player_request_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1067d2f0, 0x1590, {.first_gpr = 19});
    keep(c, {{30, c.toc(rows_slot)}, {24, item}, {31, c.toc(slot::pool)}, {25, output}});
    const auto name = frame.at(0x38), text = frame.at(0x1c8);
    const Say say{c, text};
    const Finish finish{c, output, text, name};
    if (compare(c, std::int8_t(full)) != 0) {
        Kept<29> player{c, item_value(c, item, 0, 0x1067d324)};
        Kept<28> first{c, item_value(c, item, 1, 0x1067d338)};
        Kept<27> second{c, item_value(c, item, 2, 0x1067d34c)};
        Kept<26> third{c, item_value(c, item, 3, 0x1067d360)};
        Kept<23> club_index{c, item_value(c, item, 4, 0x1067d374)};
        Kept<22> kind{c, item_value(c, item, 5, 0x1067d388)};
        Kept<21> tone{c, item_value(c, item, 6, 0x1067d39c)};
        Kept<20> detail{c, item_value(c, item, 7, 0x1067d3b0)};
        Kept<24> mood{c, item_value(c, item, 8, 0x1067d3c4)};
        Kept<19> flag{c, item_value(c, item, 9, 0x1067d3d8)};
        club_index = club_record(c, club_index);
        const std::uint32_t club = club_index;
        person_name(c, first, second, third, name, 1, player, 0x1067d40c);
        const auto s8 = sign8(c.u8(club + club_article));
        // Kind 2 reads as the player's own words, any other kind as his agent's.
        if (compare(c, kind, 2) == 0) {
            if (compare(c, mood, 3) == 0) say(0x20209, 0x1067d438, {name});
            else if (compare(c, tone, 2) == 0) say(0x20297, 0x1067d45c, {0x64, name});
            else if (compare(c, tone, 3) == 0) {
                if (compare(c, detail, 2) == 0) say(0x20343, 0x1067d484, {name});
                else if (compare(c, detail, 4) == 0) say(0x20463, 0x1067d4a4, {name});
                else say(0x20535, 0x1067d4bc, {name});
            } else say(0x20629, 0x1067d4d4, {name});
        } else if (compare(c, mood, 3) == 0) {
            if (compare(c, flag) != 0) say(0x206e5, 0x1067d514, {0x64, s8, s8, name, club + club_name});
            else say(0x207a9, 0x1067d544, {0x64, s8, s8, name, club + club_name});
        } else if (compare(c, tone, 2) == 0) say(0x20894, 0x1067d568, {0x64, name});
        else if (compare(c, tone, 3) == 0) say(0x2091c, 0x1067d58c, {0x64, name});
        else if (compare(c, detail, 4) == 0) say(0x209ef, 0x1067d5b0, {0x64, name});
        else if (compare(c, detail) == 0) say(0x20ab8, 0x1067d5d4, {0x64, name});
        else say(0x20b93, 0x1067d5f4, {0x64, name, name});
        finish(player, first, second, third, 0x1067d600, 0x1067d620, 0x1067d644);
    } else {
        Kept<19> player{c, item_value(c, item, 0, 0x1067d654)};
        Kept<20> first{c, item_value(c, item, 1, 0x1067d668)};
        Kept<21> second{c, item_value(c, item, 2, 0x1067d67c)};
        Kept<22> third{c, item_value(c, item, 3, 0x1067d690)};
        item_value(c, item, 4, 0x1067d6a4);
        Kept<23> kind{c, item_value(c, item, 5, 0x1067d6b0)};
        Kept<24> mood{c, item_value(c, item, 6, 0x1067d6c4)};
        Kept<26> detail{c, item_value(c, item, 7, 0x1067d6d8)};
        person_name(c, first, second, third, name, 5, player, 0x1067d700);
        if (compare(c, kind, 2) == 0) {
            if (compare(c, mood, 2) == 0) say(0x20c8c, 0x1067d72c, {name});
            else if (compare(c, detail, 4) == 0) say(0x20cbe, 0x1067d74c, {name});
            else say(0x20cf5, 0x1067d764, {name});
        } else if (compare(c, mood, 2) == 0) say(0x20d26, 0x1067d788, {0x64, name});
        else if (compare(c, detail, 4) == 0) say(0x20d57, 0x1067d7ac, {0x64, name});
        else say(0x20d96, 0x1067d7c8, {0x64, name});
        finish(player, first, second, third, 0x1067d7d4, 0x1067d7f4, 0x1067d818);
    }
}

// 0x1067dc10: a player and a second person quarrel, or a player is quoted. The long form names the club
// (value 4), two people with their keys (0-3 and 9-12) and the kind (5) and tone (6) of the quarrel.
void player_quarrel_story(Cpu& c, std::uint32_t, std::uint32_t item, std::uint32_t full, std::uint32_t output) {
    Frame frame(c, 0x1067dc10, 0x15e0, {.first_gpr = 18});
    keep(c, {{29, c.toc(rows_slot)}, {18, item}, {30, c.toc(slot::pool)}, {31, output}});
    const auto text = frame.at(0x220);
    const Say say{c, text};
    if (compare(c, std::int8_t(full)) != 0) {
        Kept<28> player{c, item_value(c, item, 0, 0x1067dc44)};
        Kept<27> first{c, item_value(c, item, 1, 0x1067dc58)};
        Kept<26> second{c, item_value(c, item, 2, 0x1067dc6c)};
        Kept<25> third{c, item_value(c, item, 3, 0x1067dc80)};
        Kept<24> club_index{c, item_value(c, item, 4, 0x1067dc94)};
        Kept<23> kind{c, item_value(c, item, 5, 0x1067dca8)};
        Kept<22> tone{c, item_value(c, item, 6, 0x1067dcbc)};
        item_value(c, item, 7, 0x1067dcd0);
        Kept<21> other{c, item_value(c, item, 9, 0x1067dcdc)};
        Kept<20> other_first{c, item_value(c, item, 10, 0x1067dcf0)};
        Kept<19> other_second{c, item_value(c, item, 11, 0x1067dd04)};
        Kept<18> other_third{c, item_value(c, item, 12, 0x1067dd18)};
        club_index = club_record(c, club_index);
        const std::uint32_t club = club_index;
        // The first person's name in full and short form, then the second's.
        const auto subject = frame.at(0x1a8), subject_short = frame.at(0x130), rival = frame.at(0xb8), rival_short = frame.at(0x40);
        person_name(c, first, second, third, subject, 1, player, 0x1067dd4c, 0x78);
        person_name(c, first, second, third, subject_short, 5, player, 0x1067dd6c, 0x78);
        person_name(c, other_first, other_second, other_third, rival, 1, other, 0x1067dd8c, 0x78);
        person_name(c, other_first, other_second, other_third, rival_short, 5, other, 0x1067ddac, 0x78);
        const auto s8 = sign8(c.u8(club + club_article));
        // Six arguments, or seven with the second person's short name; the texts differ by kind and tone.
        const auto pair = [&](unsigned phrase, std::uint32_t back) {
            say(phrase, back, {s8, s8, 0x64, club + club_name, subject, rival, rival_short});
        };
        const auto single = [&](unsigned phrase, std::uint32_t back) { say(phrase, back, {s8, s8, club + club_name, subject}); };
        switch (std::int32_t(kind)) {
        case 3: pair(0x20dd3, 0x1067de0c); break;
        case 4: pair(0x20eda, 0x1067de48); break;
        case 0:
            if (compare(c, tone, 2) == 0) pair(0x21018, 0x1067de98);
            else single(0x2111c, 0x1067dec4);
            break;
        case 1:
            if (compare(c, tone, 2) == 0) pair(0x2120f, 0x1067df24);
            else if (compare(c, tone, 1) == 0 || compare(c, tone, 3) == 0) single(0x21357, 0x1067df50);
            break;
        default:
            if (compare(c, tone, 2) == 0) say(0x21449, 0x1067dfa8, {s8, s8, 0x64, club + club_name, subject, rival});
            else if (compare(c, tone, 1) == 0 || compare(c, tone, 3) == 0) single(0x2156a, 0x1067dfd4);
        }
        copy_text(c, output, text, 0x1067dfe0);
        if (person_listed(c, person_row(c, player), first, second, third, 0x1067e000) != 0) {
            link_text(c, output, subject, 0x7d7, person_row(c, player), 0x1067e024);
            link_text(c, output, subject_short, 0x7d7, person_row(c, player), 0x1067e040);
        }
        if (person_listed(c, person_row(c, other), other_first, other_second, other_third, 0x1067e060) != 0) {
            link_text(c, output, rival, 0x7d7, person_row(c, other), 0x1067e084);
            link_text(c, output, rival_short, 0x7d7, person_row(c, other), 0x1067e0a0);
        }
        link_text(c, output, club + club_name, 0x7d5, club, 0x1067e0b4);
    } else {
        Kept<19> player{c, item_value(c, item, 0, 0x1067e0c4)};
        Kept<20> first{c, item_value(c, item, 1, 0x1067e0d8)};
        Kept<21> second{c, item_value(c, item, 2, 0x1067e0ec)};
        Kept<22> third{c, item_value(c, item, 3, 0x1067e100)};
        item_value(c, item, 4, 0x1067e114);
        Kept<23> kind{c, item_value(c, item, 5, 0x1067e120)};
        item_value(c, item, 6, 0x1067e134);
        Kept<18> quoted{c, item_value(c, item, 7, 0x1067e140)};
        const auto subject = frame.at(0x1a8);
        person_name(c, first, second, third, subject, 5, player, 0x1067e168);
        if (compare(c, quoted) != 0 && (compare(c, kind, 3) == 0 || compare(c, kind, 4) == 0)) say(0x21668, 0x1067e194, {subject});
        else say(0x21698, 0x1067e1ac, {subject});
        copy_text(c, output, text, 0x1067e1b8);
        if (person_listed(c, person_row(c, player), first, second, third, 0x1067e1d8) != 0)
            link_text(c, output, subject, 0x7d7, person_row(c, player), 0x1067e1fc);
    }
}
}
