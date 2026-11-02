// The title race stories: the media question, doubt and back a club.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

// Story 0x63: the newspapers ask whether the club can be a title challenger (the manager is asked when the
// club is not the user's).
void Story::title_challenge_asked() {
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(users_club), 0) != 0) return media_on_club({0x36586, 0x1069520c});
        if (at_home()) return tell({0x3661c, 0x10695264}, {home_marker(), home_marker(), home_short(), manager_full, clubs()});
        adjective_of_nation({0x10695288, 0x1b});
        outgoing(0, manager_full);
        return tell({0x366e7, 0x106952e4}, {unmarked, unmarked, home_marker(), home_marker(), nationality, home_short()});
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (at_home()) {
            if (cmp(title_holder, 0) == 0) return media_on_club({0x36355, 0x10695048});
            return media_on_competition({0x36292, 0x10694ffc}, Form::long_form);
        }
        if (cmp(title_holder, 0) == 0) return media_in_nation({0x364cf, 0x106951b4});
        return media_in_nation_on_competition({0x363f0, 0x10695124}, Form::long_form);
    }
    if (!at_home()) return media_in_nation({{0x369d1, 0x106954b4}, {0x36912, 0x10695424}});
    media_on_club({{0x36877, 0x1069538c}, {0x367d4, 0x10695340}});
}

// Story 0x61: the same question, with the media doubtful of the answer.
void Story::title_challenge_doubted() {
    if (cmp(kind, 1) != 0) {
        if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
            if (!at_home()) return media_in_nation({0x36b23, 0x106955b8});
            return media_on_club({0x36a88, 0x10695528});
        }
    } else {
        if (cmp(sb(users_club), 0) != 0) return media_on_club({0x36bda, 0x10695610});
        if (at_home()) return tell({0x36c7c, 0x10695668}, {home_marker(), home_marker(), home_short(), manager_full, clubs()});
        adjective_of_nation({0x1069568c, 0x1b});
        outgoing(0, manager_full);
        return tell({0x36d4b, 0x106956e8}, {unmarked, unmarked, home_marker(), home_marker(), nationality, home_short()});
    }
    if (at_home()) return media_on_club({0x36e3c, 0x1069573c});
    adjective_of_nation({0x10695760, 0x1c});
    tell({0x36ee1, 0x106957b4}, {unmarked, unmarked, home_marker(), home_marker(), nationality, home_short()});
}

// Story 0x5d: the club is tipped to win the title (or to retain it).
void Story::title_favourites() {
    if (cmp(kind, 0) != 0) {
        if (at_home()) return media_on_competition({{0x3762a, 0x10695e98}, {0x3758f, 0x10695e14}}, Form::long_form);
        return foreign_media_on_competition({0x10695ebc, 0x1c}, {{0x377c5, 0x10695ff8}, {0x376e5, 0x10695f5c}}, Order::club_nation_competition, Form::long_form);
    }
    if (subject_has_club_and_job()) return staff_on_competition({0x106957f8, 0x73}, {{0x3708b, 0x106959d8}, {0x36fa8, 0x106958ec}}, Form::plain);
    if (subject_is_former_player()) return former_player_on_competition({{0x3723a, 0x10695b08}, {0x37176, 0x10695a7c}}, Form::plain);
    if (at_home()) return media_on_competition({{0x37398, 0x10695c20}, {0x372ff, 0x10695b9c}}, Form::long_form);
    foreign_media_on_competition({0x10695c44, 0x1b}, {{0x374e0, 0x10695d80}, {0x3742e, 0x10695ce4}}, Order::nation_club_competition, Form::long_form);
}

// Story 0x62: the media are convinced the club cannot be stopped.
void Story::title_run_unstoppable() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({{0x37f62, 0x106965ac}, {0x37e81, 0x10696528}}, Form::long_form);
        return foreign_media_on_competition({0x106965d0, 0x1c}, {{0x38142, 0x1069670c}, {0x3803f, 0x10696670}}, Order::club_nation_competition, Form::long_form);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (subject_has_club_and_job()) return staff_on_competition({0x10696048, 0x72}, {{0x37997, 0x10696228}, {0x378a2, 0x1069613c}}, Form::plain);
        if (subject_is_former_player()) return former_player_on_competition({0x37a96, 0x106962c4}, Form::plain);
        if (!at_home()) return media_in_nation({{0x37db0, 0x10696494}, {0x37cdc, 0x10696404}});
        return media_on_club({{0x37c27, 0x1069636c}, {0x37b6f, 0x10696320}});
    }
    if (at_home()) return media_on_competition({{0x38308, 0x10696824}, {0x38241, 0x106967a0}}, Form::long_form);
    foreign_media_on_competition({0x10696848, 0x1c}, {{0x384b5, 0x10696984}, {0x383cc, 0x106968e8}}, Order::nation_club_competition, Form::long_form);
}

// Story 0x40: the media claim the club's promotion bid is stalling.
void Story::promotion_bid_stalling() {
    if (cmp(kind, 0) != 0) return media_on_competition({0x387ec, 0x10696ba0}, Form::long_form);
    if (subject_has_club_and_job()) return staff_remark({0x106969c8, 0x71}, {0x3859b, 0x10696a70});
    if (subject_is_former_player()) return tell({0x38685, 0x10696ad0}, {home_marker(), home_marker(), subject_full, home_short(), clubs()});
    media_on_club({0x38742, 0x10696b1c});
}

// Story 0x41: the club at the top of the table is said to feel the pressure.
void Story::title_pressure() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({0x38c31, 0x10696e6c}, Form::long_form);
        return media_in_nation_on_competition({0x38d1e, 0x10696f40}, Form::long_form);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (subject_has_club_and_job()) return staff_remark({0x10696bf0, 0x70}, {0x388ae, 0x10696c98});
        if (subject_is_former_player()) return tell({0x389b3, 0x10696cfc}, {home_marker(), home_marker(), subject_full, home_short(), subject_short});
        if (!at_home()) return media_in_nation({0x38b6c, 0x10696de0});
        return media_on_club({0x38ac3, 0x10696d50});
    }
    if (at_home()) return media_on_competition({{0x38ee6, 0x10697058}, {0x38e27, 0x10696fd4}}, Form::long_form);
    foreign_media_on_competition({0x1069707c, 0x1c}, {{0x39080, 0x106971b8}, {0x38f9f, 0x1069711c}}, Order::nation_club_competition, Form::long_form);
}
}
