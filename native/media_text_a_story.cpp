// The column about the player: who is writing, what they say and how. The story kind picks the
// wording, then the tone (a jump table for the kinds that have eight of them) and the player's traits.
#include "media_text_a_detail.hpp"

namespace cm::aot::media::detail::column {

// The column, by the kind of story.
void Story::write_column_by_kind() {
    if (cmp(kind, 0x23) == 0) {
        if (cmp(tone, 0x1) == 0) return say(L(0x1058, 0x1064d9b0, club_article, club_article, plain, club_name,
                player_name, clubs_base));
        if (cmp(tone, 0x1) >= 0) {
            if (cmp(tone, 0x3) >= 0) return say(L(0x1244, 0x1064da64, outlet_article, outlet_article, club_article,
                    club_article, player_name, outlet_name, club_name));
            return say(L(0x1145, 0x1064da00, club_article, club_article, plain, club_name, player_name, clubs_base));
        }
        if (cmp(tone, 0x0) < 0) return say(L(0x1244, 0x1064da64, outlet_article, outlet_article, club_article,
                club_article, player_name, outlet_name, club_name));
        if (cmp(domestic, 0) == 0) return say(L(0xf62, 0x1064d960, club_article, club_article, plain, plain, plain,
                club_name, player_name, adjective_text).adjective(0x1c, 0x1064d900, of_nation));
        return say(L(0xe8e, 0x1064d8dc, club_article, club_article, plain, club_name, player_name, clubs_base));
    }
    if (cmp(kind, 0xe) == 0) {
        if (cmp(tone, 0x2) == 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x16f4, 0x1064dd14, plain, plain, club_article, club_article,
                    adjective_text, player_name, club_name).adjective(0x1e, 0x1064dcbc, of_nation));
            return say(L(0x1634, 0x1064dc98, plain, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x2) >= 0) {
            if (cmp(tone, 0x4) == 0 || cmp(tone, 0x4) >= 0) return say(L(0x18a1, 0x1064ddb4, club_article, club_article,
                    plain, club_name, player_name, clubs_base));
            return say(L(0x17d3, 0x1064dd64, plain, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x0) == 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x13ff, 0x1064db6c, plain, plain, club_article, club_article,
                    adjective_text, player_name, club_name).adjective(0x16, 0x1064db14, of_nation));
            return say(L(0x1358, 0x1064daf0, plain, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x0) < 0) return say(L(0x18a1, 0x1064ddb4, club_article, club_article, plain, club_name,
                player_name, clubs_base));
        if (cmp(domestic, 0) == 0) return say(L(0x1573, 0x1064dc40, plain, plain, club_article, club_article,
                adjective_text, player_name, club_name).adjective(0x16, 0x1064dbe8, of_nation));
        return say(L(0x14c5, 0x1064dbc4, plain, club_article, club_article, player_name, club_name, clubs_base));
    }
    if (cmp(kind, 0xd) == 0) {
        if (cmp(tone, 0x2) == 0) return say(L(0x1b15, 0x1064df20, outlet_article, outlet_article, plain, club_article,
                club_article, outlet_name, player_name, club_name));
        if (cmp(tone, 0x2) >= 0) return say(L(0x1bf8, 0x1064df70, club_article, club_article, plain, club_name,
                player_name, clubs_base));
        if (cmp(tone, 0x0) == 0) return say(L(0x1965, 0x1064de48, outlet_article, outlet_article, club_article,
                club_article, plain, outlet_name, club_name, player_name));
        if (cmp(tone, 0x0) >= 0) return say(L(0x1a53, 0x1064deb4, outlet_article, outlet_article, plain, club_article,
                club_article, outlet_name, player_name, club_name));
        return say(L(0x1bf8, 0x1064df70, club_article, club_article, plain, club_name, player_name, clubs_base));
    }
    if (cmp(kind, 0x11) == 0 && cmp(has_match(), 0) != 0) {
        score(0x1064dfe4);
        position(0x1064e004);
        if (cmp(tone, 0x1) == 0) {
            if (cmp(goals_home(), goals_away()) > 0) {
                if (cmp(domestic, 0) == 0) {
                    say(L(0x2157, 0x1064e480, nation_article, nation_article, club_article, club_article,
                            rival_short_article, rival_short_article, position_kind, position_kind, plain, nation_name,
                            club_name, score_text, rival_short, position_text, player_name));
                    append_ending_if_level();
                    return;
                }
                say(L(0x2013, 0x1064e374, club_article, club_article, rival_short_article, rival_short_article,
                        position_kind, position_kind, plain, club_name, score_text, rival_short, position_text,
                        player_name));
                append_ending_if_level();
                return;
            }
            if (cmp(goals_home(), goals_away()) != 0) {
                if (cmp(domestic, 0) == 0) {
                    say(L(0x2676, 0x1064e838, nation_article, nation_article, club_article, club_article,
                            rival_short_article, rival_short_article, position_kind, position_kind, plain, nation_name,
                            club_name, score_text, rival_short, position_text, player_name));
                    append_ending_if_level();
                    return;
                }
                say(L(0x2539, 0x1064e72c, club_article, club_article, rival_short_article, rival_short_article,
                        position_kind, position_kind, plain, club_name, score_text, rival_short, position_text,
                        player_name));
                append_ending_if_level();
                return;
            }
            if (cmp(domestic, 0) == 0) {
                say(L(0x23ea, 0x1064e668, nation_article, nation_article, club_article, club_article,
                        rival_short_article, rival_short_article, position_kind, position_kind, plain, nation_name,
                        club_name, score_text, rival_short, position_text, player_name));
                append_ending_if_level();
                return;
            }
            say(L(0x22a2, 0x1064e55c, club_article, club_article, rival_short_article, rival_short_article,
                    position_kind, position_kind, plain, club_name, score_text, rival_short, position_text,
                    player_name));
            append_ending_if_level();
            return;
        }
        if (cmp(tone, 0x1) < 0 && cmp(tone, 0x0) >= 0) {
            if (cmp(goals_home(), goals_away()) > 0) {
                say(L(0x1ccd, 0x1064e0f8, club_article, club_article, rival_short_article, rival_short_article,
                        outlet_article, outlet_article, plain, club_name, score_text, rival_short, outlet_name,
                        player_name));
                append_ending_if_level();
                return;
            }
            if (cmp(goals_home(), goals_away()) != 0) {
                say(L(0x1ef9, 0x1064e298, club_article, club_article, rival_short_article, rival_short_article,
                        outlet_article, outlet_article, plain, club_name, score_text, rival_short, outlet_name,
                        player_name));
                append_ending_if_level();
                return;
            }
            say(L(0x1de1, 0x1064e1d4, club_article, club_article, rival_short_article, rival_short_article,
                    outlet_article, outlet_article, plain, club_name, score_text, rival_short, outlet_name,
                    player_name));
            append_ending_if_level();
            return;
        }
        if (cmp(goals_home(), goals_away()) > 0) {
            if (cmp(domestic, 0) != 0) {
                say(L(0x27c9, 0x1064e90c, club_article, club_article, rival_short_article, rival_short_article,
                        position_kind, position_kind, player_name, club_name, rival_short, score_text, position_text));
                append_ending_if_level();
                return;
            }
            clobber_register(18);
            clobber_register(14);
            say(L(0x2933, 0x1064ea20, nation_article, nation_article, plain, club_article, club_article,
                    rival_short_article, rival_short_article, position_kind, position_kind, player_name, nation_name,
                    player_short, club_name, rival_short, score_text, position_text));
            append_ending_if_level();
            return;
        }
        if (cmp(goals_home(), goals_away()) != 0) {
            if (cmp(domestic, 0) != 0) {
                say(L(0x2dfc, 0x1064ecc4, club_article, club_article, rival_short_article, rival_short_article,
                        position_kind, position_kind, player_name, club_name, score_text, rival_short, position_text));
                append_ending_if_level();
                return;
            }
            clobber_register(18);
            clobber_register(14);
            say(L(0x2f6a, 0x1064edd8, nation_article, nation_article, plain, club_article, club_article,
                    rival_short_article, rival_short_article, position_kind, position_kind, player_name, nation_name,
                    player_short, club_name, score_text, rival_short, position_text));
            append_ending_if_level();
            return;
        }
        if (cmp(domestic, 0) != 0) {
            say(L(0x2ae0, 0x1064eaf4, club_article, club_article, rival_short_article, rival_short_article,
                    position_kind, position_kind, player_name, club_name, score_text, rival_short, position_text));
            append_ending_if_level();
            return;
        }
        clobber_register(18);
        clobber_register(14);
        say(L(0x2c4e, 0x1064ec08, nation_article, nation_article, plain, club_article, club_article,
                rival_short_article, rival_short_article, position_kind, position_kind, player_name, nation_name,
                player_short, club_name, score_text, rival_short, position_text));
        append_ending_if_level();
        return;
    }
    if (cmp(kind, 0x11) == 0) return say(L(0x324d, 0x1064efb4, club_article, club_article, plain, club_name,
            player_name, clubs_base));
    if (cmp(kind, 0x0) == 0 || cmp(kind, 0x12) == 0) {
        write_column_by_tone_then_traits();
        return;
    }
    if (cmp(kind, 0xc) == 0) return say(L(0x555a, 0x10650010, outlet_article, outlet_article, club_article,
            club_article, outlet_name, player_name, club_name));
    if (cmp(kind, 0x13) == 0) {
        if (cmp((word_31d4() & 0x1), 0x1) != 0) return say(L(0x573f, 0x10650104, outlet_article, outlet_article,
                club_article, club_article, age_flag_arg, outlet_name, club_name, player_name, date_text));
        return say(L(0x5631, 0x1065008c, outlet_article, outlet_article, club_article, club_article, outlet_name,
                club_name, player_name));
    }
    if (cmp(kind, 0xa) == 0 && cmp(has_match(), 0) != 0) {
        score(0x10650178);
        if (cmp(goals_home(), goals_away()) > 0) return say(L(0x5871, 0x10650250, outlet_article, outlet_article,
                club_article, club_article, rival_short_article, rival_short_article, outlet_name, club_name,
                player_name, score_text, rival_short, manager_name));
        if (cmp(goals_home(), goals_away()) != 0) return say(L(0x5b38, 0x1065055c, outlet_article, outlet_article,
                club_article, club_article, rival_short_article, rival_short_article, outlet_name, club_name,
                player_name, score_text, rival_short, manager_name));
        say(L(0x59d4, 0x1065032c, outlet_article, outlet_article, club_article, club_article, rival_short_article,
                rival_short_article, outlet_name, club_name, player_name, score_text, rival_short, manager_name));
        level_ending(0x10650378);
        return;
    }
    if (cmp(kind, 0x9) == 0) {
        if (cmp(tone, 0x3) == 0) return say(L(0x6168, 0x1065089c, club_article, club_article, club_name, player_name,
                clubs_base));
        if (cmp(tone, 0x3) >= 0) {
            if (cmp(tone, 0x5) == 0 || cmp(tone, 0x5) >= 0) {
                if (cmp(domestic, 0) == 0) return say(L(0x6488, 0x10650a60, nation_article, nation_article,
                        club_article, club_article, nation_name, player_name, club_name));
                return say(L(0x63cf, 0x106509cc, club_article, club_article, player_name, club_name, clubs_base));
            }
            if (cmp(domestic, 0) == 0) return say(L(0x62e8, 0x10650978, plain, club_article, club_article, plain, plain,
                    player_name, club_name, adjective_text).adjective(0x1c, 0x10650918, of_nation));
            return say(L(0x6223, 0x106508f4, plain, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x1) == 0) return say(L(0x5f0b, 0x10650788, other_article, other_article, club_article,
                club_article, other_name, second_name, club_name, player_name));
        if (cmp(tone, 0x1) >= 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x608f, 0x10650850, club_article, club_article, plain, player_name,
                    club_name, adjective_text).adjective(0x16, 0x10650800, of_nation));
            return say(L(0x5fd6, 0x106507dc, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x0) >= 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x5dc4, 0x106506f8, other_article, other_article, plain, plain,
                    club_article, club_article, other_name, second_name, adjective_text, player_name,
                    club_name).adjective(0x1c, 0x10650654, of_nation));
            return say(L(0x5c9f, 0x10650630, other_article, other_article, club_article, club_article, other_name,
                    second_name, player_name, club_name));
        }
        if (cmp(domestic, 0) == 0) return say(L(0x6488, 0x10650a60, nation_article, nation_article, club_article,
                club_article, nation_name, player_name, club_name));
        return say(L(0x63cf, 0x106509cc, club_article, club_article, player_name, club_name, clubs_base));
    }
    if (cmp(kind, 0x22) == 0) {
        if (cmp(tone, 0x1) == 0 || cmp(tone, 0x1) >= 0 || cmp(tone, 0x0) < 0) return say(L(0x6695, 0x10650b30,
                club_article, club_article, club_name, manager_name, player_name, manager_short));
        return say(L(0x6560, 0x10650adc, club_article, club_article, plain, club_name, player_name, manager_name,
                player_short));
    }
    if (cmp(kind, 0x21) == 0) {
        if (cmp(tone, 0x2) == 0) return say(L(0x69dd, 0x10650bf4, plain, player_name, manager_name, player_short,
                manager_short));
        if (cmp(tone, 0x2) >= 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x6c49, 0x10650d04, club_article, club_article, nation_article,
                    nation_article, plain, club_name, player_name, nation_name, player_short, manager_name));
            return say(L(0x6ae5, 0x10650c58, club_article, club_article, plain, club_name, player_name, player_short,
                    manager_name));
        }
        if (cmp(tone, 0x0) == 0) return say(L(0x67d4, 0x10650b74, plain, player_name, manager_name));
        if (cmp(tone, 0x0) >= 0) return say(L(0x6897, 0x10650bd0, club_article, club_article, plain, club_name,
                player_name, manager_name, player_short));
        if (cmp(domestic, 0) == 0) return say(L(0x6c49, 0x10650d04, club_article, club_article, nation_article,
                nation_article, plain, club_name, player_name, nation_name, player_short, manager_name));
        return say(L(0x6ae5, 0x10650c58, club_article, club_article, plain, club_name, player_name, player_short,
                manager_name));
    }
    if (cmp(kind, 0x8) == 0) {
        if (cmp(tone, 0x1) == 0 || cmp(tone, 0x1) >= 0 || cmp(tone, 0x0) < 0) return say(L(0x6f4d, 0x10650e84,
                club_article, outlet_article, outlet_article, club_article, plain, outlet_name, player_name,
                club_name));
        if (cmp(domestic, 0) == 0) return say(L(0x6e80, 0x10650e18, nation_article, nation_article, club_article,
                club_article, plain, nation_name, player_name, club_name));
        return say(L(0x6dcf, 0x10650d7c, club_article, club_article, plain, player_name, club_name, clubs_base));
    }
    if (cmp(kind, 0x1) == 0) {
        if (cmpl(tone, 0x6) > 0) return;
        switch (tone) {
        case 0: {
            return say(L(0x7023, 0x10650f14, outlet_article, outlet_article, club_article, club_article, plain,
                    outlet_name, player_name, club_name));
        }
        case 1: {
            return say(L(0x7129, 0x10650fd0, outlet_article, outlet_article, plain, person_article, person_article,
                    club_article, club_article, outlet_name, player_name, person_text, club_name));
        }
        case 2: {
            if (cmp(domestic, 0) == 0) return say(L(0x7307, 0x106510b8, nation_article, nation_article, club_article,
                    club_article, nation_name, player_name, club_name));
            return say(L(0x7247, 0x10651024, club_article, club_article, player_name, club_name, clubs_base));
        }
        case 3: {
            if (cmp(domestic, 0) == 0) return say(L(0x74c2, 0x106511b0, plain, team_article, team_article,
                    adjective_text, player_name, team_name).adjective(0x1d, 0x10651148, of_nation));
            return say(L(0x73e3, 0x10651124, team_article, team_article, player_name, team_name, teams_base,
                    teams_base));
        }
        case 4: {
            return say(L(0x75bd, 0x10651264, outlet_article, outlet_article, club_article, club_article, person_article,
                    person_article, outlet_name, club_name, player_name, person_text));
        }
        case 5: {
            return say(L(0x7691, 0x10651300, outlet_article, outlet_article, club_article, club_article, plain, plain,
                    outlet_name, club_name, player_name, adjective_text).adjective(0x2, 0x10651288, of_person));
        }
        default: {  // tone 6 (the guard above leaves 0-6)
            return say(L(0x779f, 0x106513b4, outlet_article, outlet_article, club_article, club_article, person_article,
                    person_article, outlet_name, club_name, player_name, person_text));
        }
        }
    }
    if (cmp(kind, 0x25) == 0) return say(L(0x78a1, 0x1065144c, outlet_article, outlet_article, plain,
            club_short_article, club_short_article, age_flag_arg, plain, outlet_name, player_name, club_short,
            date_text, third_name));
    if (cmp(kind, 0x1e) == 0) {
        if (cmp(tone, 0x1) == 0 || cmp(tone, 0x1) >= 0 || cmp(tone, 0x0) < 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x7ca2, 0x10651610, plain, club_article, club_article,
                    adjective_text, player_name, club_name).adjective(0x19, 0x106515c0, of_nation));
            return say(L(0x7bc6, 0x1065159c, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(domestic, 0) == 0) return say(L(0x7abe, 0x10651548, plain, plain, plain, club_article, club_article,
                adjective_text, player_name, club_name).adjective(0x19, 0x106514e8, of_nation));
        return say(L(0x79d8, 0x106514c4, plain, club_article, club_article, player_name, club_name, clubs_base));
    }
    if (cmp(kind, 0x7) == 0) {
        if (cmp(tone, 0x0) == 0) return say(L(0x7da0, 0x10651688, outlet_article, outlet_article, club_article,
                club_article, outlet_name, player_name, club_name));
        if (cmp(domestic, 0) == 0) return say(L(0x7f0a, 0x10651770, nation_article, nation_article, club_article,
                club_article, nation_name, club_name, player_name));
        return say(L(0x7e56, 0x106516dc, club_article, club_article, club_name, player_name, clubs_base));
    }
    if (cmp(kind, 0x2) == 0) return say(L(0x7fd4, 0x106517e4, outlet_article, outlet_article, plain, club_article,
            club_article, outlet_name, player_name, club_name));
    if (cmp(kind, 0x20) == 0) {
        if (cmp(tone, 0x1) == 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x8226, 0x10651934, plain, plain, club_article, club_article,
                    adjective_text, player_name, club_name).adjective(0x1c, 0x106518d8, of_nation));
            return say(L(0x8185, 0x106518b4, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x1) >= 0 || cmp(tone, 0x0) < 0) return say(L(0x82e9, 0x10651984, club_article, club_article,
                player_name, club_name, clubs_base));
        return say(L(0x808a, 0x1065185c, club_article, club_article, plain, club_name, player_name));
    }
    if (cmp(kind, 0x3) == 0) {
        if (cmp(tone, 0x2) == 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x86a2, 0x10651bc8, nation_article, nation_article, club_article,
                    club_article, nation_name, player_name, club_name));
            return say(L(0x85a1, 0x10651b30, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x2) >= 0) {
            if (cmp(tone, 0x4) == 0 || cmp(tone, 0x4) >= 0) return say(L(0x8967, 0x10651d08, outlet_article,
                    outlet_article, club_article, club_article, outlet_name, player_name, club_name));
            if (cmp(domestic, 0) == 0) return say(L(0x8882, 0x10651ca0, plain, plain, club_article, club_article,
                    adjective_text, player_name, club_name).adjective(0x1b, 0x10651c44, of_nation));
            return say(L(0x87c3, 0x10651c20, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(tone, 0x0) == 0) return say(L(0x8386, 0x10651a70, outlet_article, outlet_article, club_article,
                club_article, person_article, person_article, outlet_name, player_name, club_name, person_text));
        if (cmp(tone, 0x0) >= 0) return say(L(0x84b5, 0x10651ad8, outlet_article, outlet_article, club_article,
                club_article, outlet_name, player_name, club_name));
        return say(L(0x8967, 0x10651d08, outlet_article, outlet_article, club_article, club_article, outlet_name,
                player_name, club_name));
    }
    if (cmp(kind, 0x4) == 0) {
        if (cmp(tone, 0x1) == 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x8b82, 0x10651dfc, nation_article, nation_article, nation_name,
                    player_name, nation));
            return say(L(0x8ae4, 0x10651da0, player_name));
        }
        if (cmp(tone, 0x1) < 0 && cmp(tone, 0x0) >= 0) return say(L(0x8a40, 0x10651d80, plain, club_article,
                club_article, player_name, club_name));
        if (cmp(domestic, 0) == 0) return say(L(0x8cef, 0x10651ed4, plain, plain, club_article, club_article,
                adjective_text, player_name, club_name).adjective(0x1b, 0x10651e78, of_nation));
        return say(L(0x8c3d, 0x10651e54, club_article, club_article, player_name, club_name, clubs_base));
    }
    if (cmp(kind, 0x5) == 0) {
        if (cmp(tone, 0x0) == 0) {
            if (cmp(domestic, 0) == 0) return say(L(0x8e6c, 0x10651fd8, nation_article, nation_article, club_article,
                    club_article, nation_name, player_name, club_name));
            return say(L(0x8dc3, 0x10651f40, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (cmp(domestic, 0) == 0) return say(L(0x8ff7, 0x106520d4, nation_article, nation_article, plain, club_article,
                club_article, nation_name, player_name, club_name));
        return say(L(0x8f35, 0x10652034, plain, club_article, club_article, player_name, club_name));
    }
    if (cmp(kind, 0x6) == 0) {
        if (cmp(domestic, 0) == 0) return say(L(0x91a8, 0x106521c0, plain, plain, plain, club_article, club_article,
                adjective_text, player_name, club_name).adjective(0x1e, 0x1065215c, of_nation));
        return say(L(0x90d9, 0x10652138, plain, club_article, club_article, player_name, club_name));
    }
    if (cmp(kind, 0x10) != 0) return say(L(0x9411, 0x106522c8, player_name).plain());
    if (cmp(domestic, 0) == 0) return say(L(0x9344, 0x106522b0, plain, plain, plain, club_article, club_article,
            adjective_text, player_name, club_article).adjective(0x1e, 0x10652248, of_nation));
    return say(L(0x9299, 0x10652224, plain, club_article, club_article, player_name, club_name));
}

// Kinds 0 and 0x12: the eight tones first, then the player's traits one by one; a player with none of them
// takes his tone from his number and goes round again.
void Story::write_column_by_tone_then_traits() {
    for (;;) {
        if (cmpl(tone, 0x7) <= 0) {
            switch (tone) {
            case 0: {
                if (cmp(domestic, 0) == 0) return say(L(0x33e5, 0x1064f0d4, nation_article, nation_article,
                        club_article, club_article, plain, nation_name, club_name, player_name));
                return say(L(0x330c, 0x1064f038, club_article, club_article, plain, club_name, player_name,
                        clubs_base));
            }
            case 1: {
                return say(L(0x34c1, 0x1064f140, outlet_article, outlet_article, plain, club_article, club_article,
                        outlet_name, player_name, club_name));
            }
            case 2: {
                return say(L(0x3581, 0x1064f190, club_article, club_article, plain, club_name, player_name,
                        clubs_base));
            }
            case 3: {
                return say(L(0x3639, 0x1064f1e0, club_article, club_article, plain, club_name, player_name,
                        clubs_base));
            }
            case 4: {
                if (cmp(domestic, 0) == 0) return say(L(0x378a, 0x1064f2b4, plain, club_article, club_article, plain,
                        adjective_text, club_name, player_name).adjective(0x1e, 0x1064f25c, of_nation));
                return say(L(0x36fd, 0x1064f238, club_article, club_article, plain, club_name, player_name,
                        clubs_base));
            }
            case 5: {
                if (cmp(domestic, 0) == 0) return say(L(0x392f, 0x1064f3c4, nation_article, nation_article,
                        club_article, club_article, plain, plain, nation_name, club_name, manager_name, player_name));
                return say(L(0x3836, 0x1064f318, club_article, club_article, plain, plain, club_name, manager_name,
                        player_name));
            }
            case 6: {
                if (cmp(domestic, 0) == 0) return say(L(0x3b4c, 0x1064f4c4, nation_article, nation_article,
                        club_article, club_article, plain, nation_name, club_name, manager_name, player_name));
                return say(L(0x3a46, 0x1064f420, club_article, club_article, plain, club_name, manager_name,
                        player_name));
            }
            default: {  // tone 7 (the guard above leaves 0-7)
                if (cmp(domestic, 0) == 0) return say(L(0x3daf, 0x1064f5ac, plain, plain, club_article, club_article,
                        plain, adjective_text, club_name, manager_name, player_name).adjective(0x1d, 0x1064f544,
                        of_nation));
                return say(L(0x3c70, 0x1064f520, club_article, club_article, plain, club_name, manager_name,
                        player_name));
            }
            }
        }
        if (has_all_traits_unsigned(0x400200, 0x400000)) return say(L(0x3f10, 0x1064f630, outlet_article,
                outlet_article, club_article, club_article, plain, outlet_name, player_name, club_name));
        if (has_all_traits_unsigned(0x102200, 0x100000)) return say(L(0x3fe3, 0x1064f6c4, outlet_article,
                outlet_article, plain, club_article, club_article, plain, outlet_name, player_name, club_name,
                player_short));
        if (has_all_traits_unsigned(0x104200, 0x100000)) return say(L(0x4122, 0x1064f750, outlet_article,
                outlet_article, plain, club_article, club_article, outlet_name, player_name, club_name, player_short));
        if (has_all_traits_unsigned(0x100200, 0x100000)) return say(L(0x4284, 0x1064f7d4, outlet_article,
                outlet_article, plain, club_article, club_article, outlet_name, player_name, club_name));
        if (has_all_traits(0x480)) return say(L(0x434b, 0x1064f844, outlet_article, outlet_article, club_article,
                club_article, outlet_name, player_name, club_name));
        if (has_all_traits(0x500)) return say(L(0x442d, 0x1064f8b4, club_article, club_article, outlet_article,
                outlet_article, club_name, outlet_name, player_name));
        if (has_all_traits_unsigned(0x20820, 0x20000)) return say(L(0x45c3, 0x1064f938, plain, club_article,
                club_article, outlet_article, outlet_article, player_name, club_name, outlet_name));
        if (has_all_traits(0x1020)) return say(L(0x46c6, 0x1064f9c0, plain, club_article, club_article, outlet_article,
                outlet_article, plain, player_name, club_name, outlet_name, player_short));
        if (has_all_traits_unsigned(0x20810, 0x20000)) {
            if (cmp(domestic, 0) == 0) return say(L(0x48ef, 0x1064faac, plain, club_article, club_article, plain,
                    player_name, club_name, adjective_text).adjective(0x1d, 0x1064fa54, of_nation));
            return say(L(0x47f7, 0x1064fa30, plain, club_article, club_article, player_name, club_name, clubs_base));
        }
        if (has_all_traits_unsigned(0x10001, 0x10000)) {
            if (cmp(domestic, 0) == 0) return say(L(0x4b28, 0x1064fbb8, club_article, club_article, nation_article,
                    nation_article, club_name, player_name, nation_name, player_short));
            return say(L(0x49fb, 0x1064fb1c, club_article, club_article, club_name, player_name, player_short,
                    clubs_base));
        }
        if (has_any_trait(0x1)) {
            if (cmp(domestic, 0) == 0) return say(L(0x4da2, 0x1064fc94, club_article, club_article, plain, club_name,
                    player_name, adjective_text, player_short).adjective(0x1e, 0x1064fc3c, of_nation));
            return say(L(0x4c73, 0x1064fc18, club_article, club_article, club_name, player_name, player_short,
                    clubs_base));
        }
        if (has_all_traits(0xa)) return say(L(0x4eee, 0x1064fd0c, club_article, club_article, outlet_article,
                outlet_article, club_name, player_name, outlet_name, player_short));
        if (has_all_traits(0x8)) {
            if (cmp(domestic, 0) == 0) return say(L(0x50cb, 0x1064fe00, club_article, club_article, nation_article,
                    nation_article, club_name, player_name, nation_name));
            return say(L(0x502a, 0x1064fd6c, club_article, club_article, club_name, player_name, clubs_base));
        }
        if (has_all_traits(0x2) && cmp(domestic, 0) != 0) return say(L(0x519b, 0x1064fe68, club_article, club_article,
                plain, club_name, player_name, player_short));
        if (has_all_traits(0x4000)) return say(L(0x52ab, 0x1064fec0, club_article, club_article, club_name, player_name,
                clubs_base));
        if (has_all_traits_unsigned(0x20000, 0x20000)) return say(L(0x5353, 0x1064ff1c, club_article, club_article,
                club_name, player_name, clubs_base));
        if (!has_all_traits_unsigned(0x40000, 0x40000) || has_any_trait(0x10) || has_any_trait(0x20)) {
            reseed_tone();
            continue;
        }
        return say(L(0x5401, 0x1064ff8c, club_article, club_article, club_name, player_name, player_short, clubs_base));
    }
}

// A level match gets its closing sentence.
void Story::append_ending_if_level() {
    if (cmp(goals_home(), goals_away()) != 0) return;
    level_ending(0x1064ee3c);
}

// The closing sentence of a level match, for the story's result 1-4 (text_shared.hpp), put in the reply
// text and appended to the story's.
void Story::level_ending(std::uint32_t first_back) {
    text::match_ending(c, result, first_back, at(unsigned(reply_text)), at(unsigned(story_text)), [&] {
        return std::array{value(club_article), value(club_article), value(club_name), value(clubs_base)};
    });
}
}
