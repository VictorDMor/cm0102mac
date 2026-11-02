// The club stories: runs of results, the table, the title and the tournament.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

// Story 0x50: an unbeaten run ends with a defeat. The wording depends on whether the subject (the opposition's
// player) is named and on the defeat: heavy, low-scoring or the rest.
void Story::unbeaten_run_ends() {
    if (cmp(sb(has_match), 0) == 0) return no_match();
    score(0x1068ad1c);
    if (cmp(kind, 1) != 0) {
        if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
            if (cmp(subject_id, -1) <= 0) {
                const auto w = by_margin({{{0x2b979, 0x1068b030}, {0x2baf4, 0x1068b18c}, {0x2ba2e, 0x1068b0ec}}});
                outgoing(0, score_text);
                outgoing(1, rival_long());
                return tell(w, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), run_length});
            }
            const auto w = by_margin({{{0x2b6b9, 0x1068ae00}, {0x2b896, 0x1068af74}, {0x2b79e, 0x1068aec8}}});
            outgoing(0, run_length);
            outgoing(1, score_text);
            outgoing(2, subject_full);
            outgoing(3, rival_long());
            return tell(w, {home_marker(), home_marker(), unmarked, rival_long_marker(), rival_long_marker(), home_short()});
        }
    } else {
        if (cmp(subject_id, -1) <= 0) {
            const auto w = by_margin({{{0x2bf3c, 0x1068b564}, {0x2c0ae, 0x1068b6c0}, {0x2bfee, 0x1068b620}}});
            outgoing(0, rival_long());
            outgoing(1, score_text);
            return tell(w, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), run_length, home_short()});
        }
        const auto w = by_margin({{{0x2bba7, 0x1068b2a4}, {0x2be0f, 0x1068b4a8}, {0x2bcd4, 0x1068b3b4}}});
        outgoing(0, run_length);
        outgoing(1, home_short());
        outgoing(2, rival_long());
        outgoing(3, score_text);
        outgoing(4, rival_long());
        outgoing(5, subject_full);
        const auto home_row = scratch<13>(home());  // (the first marker is read through it)
        return tell(w, {club_view(home_row).short_marker(), home_marker(), rival_long_marker(), rival_long_marker(), rival_marker(), rival_marker()});
    }
    if (cmp(subject_id, -1) <= 0) {
        const auto w = by_margin({{{0x2c48a, 0x1068b9b4}, {0x2c61f, 0x1068bb10}, {0x2c555, 0x1068ba70}}});
        outgoing(0, score_text);
        outgoing(1, rival_long());
        return tell(w, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), run_length});
    }
    const auto w = by_margin({{{0x2c160, 0x1068b78c}, {0x2c37c, 0x1068b8f8}, {0x2c264, 0x1068b850}}});
    outgoing(0, score_text);
    outgoing(1, rival_long());
    outgoing(2, subject_full);
    tell(w, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), run_length});
}

// Story 0x51: a winless run ends with a win.
void Story::winless_run_ends() {
    if (cmp(sb(has_match), 0) == 0) return no_match();
    score(0x1068bb94);
    if (cmp(kind, 1) == 0) {
        if (cmp(subject_id, -1) <= 0) {
            outgoing(0, score_text);
            outgoing(1, rival_long());
            return tell({0x2c98e, 0x1068be44}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), run_length});
        }
        outgoing(0, score_text);
        outgoing(1, rival_long());
        outgoing(2, run_length);
        return tell({0x2c8b2, 0x1068bda4}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), subject_full, home_short()});
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(subject_id, -1) <= 0) {
            outgoing(0, rival_long());
            outgoing(1, score_text);
            return tell({0x2c805, 0x1068bcf8}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), run_length});
        }
        outgoing(0, rival_long());
        outgoing(1, score_text);
        outgoing(2, subject_full);
        return tell({0x2c6df, 0x1068bc58}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), run_length});
    }
    // The rival's long marker is read through the club table and the rival's offset in it.
    if (cmp(subject_id, -1) <= 0) {
        outgoing(0, score_text);
        outgoing(1, run_length);
        const auto rival_offset = scratch<13>(rival_id() * club_size);
        const auto table = scratch<14>(clubs());
        return tell({0x2cb30, 0x1068bf8c}, {home_marker(), home_marker(), club_view(table + rival_offset).long_marker(), rival_long_marker(), home_short(), rival_short()});
    }
    outgoing(0, score_text);
    outgoing(1, run_length);
    outgoing(2, subject_full);
    const auto table = scratch<13>(clubs());
    tell({0x2ca3b, 0x1068bef0}, {home_marker(), home_marker(), rival_long_marker(), club_view(table + rival_id() * club_size).long_marker(), home_short(), rival_short()});
}

// Story 0x3f: a club in a relegation battle.
void Story::relegation_battle() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({0x2cf9b, 0x1068c33c}, Form::plain);
        return foreign_media_on_competition({0x1068c360, 0x1c}, {0x2d0ab, 0x1068c3f8}, Order::club_competition_nation, Form::plain);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (subject_has_club_and_job()) return staff_on_competition({0x1068bfe0, 0x85}, {0x2cbd1, 0x1068c0cc}, Form::plain);
        if (subject_is_former_player()) return former_player_on_competition({0x2ccd3, 0x1068c168}, Form::plain);
        if (at_home()) return media_on_competition({0x2cdaf, 0x1068c1f4}, Form::plain);
        return foreign_media_on_competition({0x1068c218, 0x1c}, {0x2ce94, 0x1068c2b0}, Order::club_nation_competition, Form::plain);
    }
    if (!at_home()) return tell({0x2d297, 0x1068c4dc}, {home_marker(), home_marker(), nation_marker(), nation_marker(), home_short(), nation_name()});
    media_on_club({0x2d1dd, 0x1068c44c});
}

// Story 0x5f: a club tipped for promotion.
void Story::promotion_tipped() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({0x2d6a2, 0x1068c828}, Form::plain);
        return media_in_nation_on_competition({0x2d75d, 0x1068c8fc}, Form::plain);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (subject_has_club_and_job()) return staff_on_competition({0x1068c530, 0x84}, {0x2d363, 0x1068c61c}, Form::plain);
        if (subject_is_former_player()) return former_player_on_competition({0x2d452, 0x1068c6b8}, Form::plain);
        if (!at_home()) return media_in_nation({0x2d5d1, 0x1068c79c});
        return media_on_club({0x2d51c, 0x1068c70c});
    }
    if (at_home()) return media_on_competition({0x2d834, 0x1068c988}, Form::long_form);
    foreign_media_on_competition({0x1068c9ac, 0x1c}, {0x2d914, 0x1068ca44}, Order::nation_club_competition, Form::long_form);
}

// Story 0x60: a club climbing out of the relegation zone.
void Story::relegation_escape_tipped() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({0x2dd4c, 0x1068cd90}, Form::plain);
        return media_in_nation_on_competition({0x2de0b, 0x1068ce64}, Form::plain);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (subject_has_club_and_job()) return staff_on_competition({0x1068ca98, 0x83}, {0x2da16, 0x1068cb84}, Form::plain);
        if (subject_is_former_player()) return former_player_on_competition({0x2db1a, 0x1068cc20}, Form::plain);
        if (!at_home()) return media_in_nation({0x2dc94, 0x1068cd04});
        return media_on_club({0x2dbf8, 0x1068cc74});
    }
    if (at_home()) return media_on_competition({0x2dee6, 0x1068cef0}, Form::long_form);
    foreign_media_on_competition({0x1068cf14, 0x1c}, {0x2dfca, 0x1068cfac}, Order::nation_club_competition, Form::long_form);
}

// Story 0x42: the media write off a club's chances in the title race (a club that holds the title is written off
// from retaining it).
void Story::title_written_off() {
    if (cmp(kind, 0) != 0) {
        if (at_home()) return media_on_competition({{0x2e6fc, 0x1068d5fc}, {0x2e63a, 0x1068d578}}, Form::long_form);
        return foreign_media_on_competition({0x1068d620, 0x1c}, {{0x2e88f, 0x1068d75c}, {0x2e7ab, 0x1068d6c0}}, Order::club_competition_nation, Form::long_form);
    }
    if (subject_has_club_and_job()) return staff_on_competition({0x1068cff0, 0x82}, {{0x2e1ba, 0x1068d1d0}, {0x2e0d0, 0x1068d0e4}}, Form::plain);
    if (subject_is_former_player()) return former_player_on_competition({0x2e29f, 0x1068d26c}, Form::plain);
    if (at_home()) return media_on_competition({{0x2e3fe, 0x1068d384}, {0x2e354, 0x1068d300}}, Form::long_form);
    foreign_media_on_competition({0x1068d3a8, 0x1b}, {{0x2e571, 0x1068d4e4}, {0x2e4ae, 0x1068d448}}, Order::nation_club_competition, Form::long_form);
}

// Story 0x43: the media write a mid-table club off as dull.
void Story::midtable_written_off() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({0x2ec87, 0x1068daa8}, Form::long_form);
        return foreign_media_on_competition({0x1068dacc, 0x1c}, {0x2ed5e, 0x1068db64}, Order::club_nation_competition, Form::long_form);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (subject_has_club_and_job()) return staff_on_competition({0x1068d7b0, 0x81}, {0x2e960, 0x1068d89c}, Form::plain);
        if (subject_is_former_player()) return former_player_on_competition({0x2ea60, 0x1068d938}, Form::plain);
        // The manager is the one blamed.
        if (at_home()) return tell({0x2eb27, 0x1068d994}, {home_marker(), home_marker(), unmarked, home_short(), manager_full});
        adjective_of_nation({0x1068d9b8, 0x1c});
        outgoing(0, nationality);
        outgoing(1, manager_full);
        return tell({0x2ebc7, 0x1068da1c}, {home_marker(), home_marker(), unmarked, unmarked, unmarked, home_short()});
    }
    if (at_home()) return media_on_club({0x2ee55, 0x1068dbb8});
    adjective_of_nation({0x1068dbdc, 0x1c});  // (the adjective is built and not used)
    media_in_nation({0x2ef2b, 0x1068dc68});
}

namespace {
// The leaves of the tournament tips (stories 0x44 and 0x5a) for a club that does not hold the title, and for one
// that does. Each is: what the media say, what a former player says, and what a club's manager says (with the
// call that spells his job); for a club of a host nation, of a nation with one other host, of one with two.
constexpr HostTips written_off_fresh{
    {{0x2f97e, 0x1068e850}, {0x2f8c0, 0x1068e7cc}, {0x2f7e0, 0x1068e730}, Form::long_form, Form::long_form, Form::long_form, {0x1068e644, 0x7d}},
    {{0x2fef5, 0x1068ef34}, {0x2fe21, 0x1068ee6c}, {0x2fd2b, 0x1068ed8c}, Form::long_form, Form::long_form, Form::long_form, {0x1068ec5c, 0x7b}},
    {{0x2fc3f, 0x1068ec2c}, {0x2fb42, 0x1068eb20}, {0x2fa2b, 0x1068e9fc}, Form::long_form, Form::long_form, Form::long_form, {0x1068e888, 0x7c}}};
constexpr HostTips written_off_holding{
    {{0x2f1b8, 0x1068def8}, {0x2f0f8, 0x1068de74}, {0x2f01e, 0x1068ddd8}, Form::long_form, Form::long_form, Form::long_form, {0x1068dcec, 0x80}},
    {{0x2f723, 0x1068e5dc}, {0x2f64d, 0x1068e514}, {0x2f55d, 0x1068e434}, Form::long_form, Form::long_form, Form::long_form, {0x1068e304, 0x7e}},
    {{0x2f477, 0x1068e2d4}, {0x2f378, 0x1068e1c8}, {0x2f25f, 0x1068e0a4}, Form::long_form, Form::long_form, Form::long_form, {0x1068df30, 0x7f}}};
}

// Story 0x44: the media do not see a country as a contender for a tournament. The phrases name countries where
// the other stories name clubs (the club row is the national side's, inferred); the hosts' own tournament is
// told differently.
void Story::tournament_written_off() {
    if (cmp(kind, 0) != 0) {
        if (cmp(title_holder, 0) == 0) {
            if (!club_in_host_nation()) {
                if (cmp(host_two, -1) <= 0) return one_host_media({0x303dc, 0x1068f46c}, Form::long_form);
                return two_hosts_media({0x302ee, 0x1068f3a4}, Form::long_form);
            }
        } else {
            if (club_in_host_nation()) return media_on_competition({0x2ffb8, 0x1068eff8}, Form::long_form);
            if (cmp(host_two, -1) <= 0) return one_host_media({0x30173, 0x1068f1d4}, Form::long_form);
            return two_hosts_media({0x30072, 0x1068f10c}, Form::long_form);
        }
    } else {
        return host_tips(cmp(title_holder, 0) == 0 ? written_off_fresh : written_off_holding);
    }
    media_on_competition({0x3024b, 0x1068f290}, Form::long_form);
}
}
