// The tournament, cup and transfer-spending stories, and the pundits' complaints and praise.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

namespace {
// The leaves of the tournament-favourite tips (see written_off_fresh in the first stories file): for a club that
// does not hold the title and for one that does. The marker widths differ from story to story where the
// original's phrases did.
constexpr HostTips favourites_fresh{
    {{0x30e77, 0x10690064}, {0x30db3, 0x1068ffe0}, {0x30cd5, 0x1068ff44}, Form::plain, Form::long_form, Form::long_form, {0x1068fe58, 0x77}},
    {{0x3143a, 0x10690748}, {0x3134e, 0x10690680}, {0x31248, 0x106905a0}, Form::plain, Form::plain, Form::plain, {0x10690470, 0x75}},
    {{0x3115c, 0x10690440}, {0x31047, 0x10690334}, {0x30f18, 0x10690210}, Form::plain, Form::plain, Form::plain, {0x1069009c, 0x76}}};
constexpr HostTips favourites_holding{
    {{0x30641, 0x1068f70c}, {0x3057b, 0x1068f688}, {0x3049b, 0x1068f5ec}, Form::plain, Form::long_form, Form::long_form, {0x1068f500, 0x7a}},
    {{0x30c10, 0x1068fdf0}, {0x30b22, 0x1068fd28}, {0x30a1a, 0x1068fc48}, Form::plain, Form::plain, Form::plain, {0x1068fb18, 0x78}},
    {{0x3092c, 0x1068fae8}, {0x30815, 0x1068f9dc}, {0x306e4, 0x1068f8b8}, Form::plain, Form::plain, Form::plain, {0x1068f744, 0x79}}};
}

// Story 0x5a: a country is touted as a winner of the tournament it hosts, or of one it may retain. Told of the
// nation's club (the club row is the national side's, inferred).
void Story::tournament_favourites() {
    if (cmp(kind, 1) == 0) {
        if (cmp(title_holder, 0) == 0) {
            if (club_in_host_nation()) return media_on_competition({0x31838, 0x10690b70}, Form::plain);
            return media_on_competition({0x318c6, 0x10690bf4}, Form::plain);
        }
        if (club_in_host_nation()) {
            outgoing(0, home_short());
            outgoing(1, competition_name());
            outgoing(2, home_short());
            return tell({0x314fd, 0x10690850}, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_marker(), home_marker()});
        }
        const auto first = host_nation(host_one), second = host_nation(host_two);
        if (cmp(host_two, -1) <= 0) {
            outgoing(0, first.marker());
            outgoing(1, first.marker());
            outgoing(2, home_short());
            outgoing(3, competition_name());
            outgoing(4, home_short());
            outgoing(5, first.name());
            const auto home_row = scratch<13>(home());  // (the first marker is read through it)
            return tell({0x31727, 0x10690ab4}, {club_view(home_row).short_marker(), home_marker(), competition_marker(), competition_marker(), home_marker(), home_marker()});
        }
        outgoing(0, first.marker());
        outgoing(1, first.marker());
        outgoing(2, second.marker());
        outgoing(3, second.marker());
        outgoing(4, home_short());
        outgoing(5, competition_name());
        outgoing(6, home_short());
        outgoing(7, first.name());
        outgoing(8, second.name());
        const auto home_row = scratch<13>(home());
        return tell({0x315ed, 0x106909a8}, {club_view(home_row).short_marker(), home_marker(), competition_marker(), competition_marker(), home_marker(), home_marker()});
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) return host_tips(cmp(title_holder, 0) == 0 ? favourites_fresh : favourites_holding);
    // Any other result: the outlet that carries the story is named.
    if (cmp(title_holder, 0) != 0) {
        const auto w = club_in_host_nation() ? Wording{0x31947, 0x10690d1c} : Wording{0x31a9a, 0x10690e04};
        outgoing(0, home_marker());
        outgoing(1, home_marker());
        outgoing(2, home_short());
        outgoing(3, competition_long_name());
        outgoing(4, outlet_text);
        outgoing(5, home_short());
        return tell(w, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
    }
    if (!club_in_host_nation()) {
        const auto first = host_nation(host_one), second = host_nation(host_two);
        if (cmp(host_two, -1) <= 0) {
            outgoing(0, sb(outlet_flag()));
            outgoing(1, sb(outlet_flag()));
            outgoing(2, home_marker());
            outgoing(3, home_marker());
            outgoing(4, home_short());
            outgoing(5, competition_long_name());
            outgoing(6, first.name());
            outgoing(7, outlet_text);
            outgoing(8, home_short());
            const auto home_row = scratch<13>(home());
            return tell({0x31ebc, 0x10691184}, {club_view(home_row).short_marker(), home_marker(), competition_long_marker(), competition_long_marker(), first.marker(), first.marker()});
        }
        outgoing(0, second.marker());
        outgoing(1, second.marker());
        outgoing(2, sb(outlet_flag()));
        outgoing(3, sb(outlet_flag()));
        outgoing(4, home_marker());
        outgoing(5, home_marker());
        outgoing(6, home_short());
        outgoing(7, competition_long_name());
        outgoing(8, first.name());
        outgoing(9, second.name());
        outgoing(10, outlet_text);
        outgoing(11, home_short());
        // r16 and r14 hold the hosts' names as offsets in the nation table, r17 the club table, and r13 the home club.
        scratch<16>(first.name() - nation_table());
        scratch<14>(second.name() - nation_table());
        scratch<17>(clubs());
        const auto home_row = scratch<13>(home());
        return tell({0x31d26, 0x10691058}, {club_view(home_row).short_marker(), home_marker(), competition_long_marker(), competition_long_marker(), first.marker(), first.marker()});
    }
    outgoing(0, home_short());
    outgoing(1, competition_long_name());
    outgoing(2, outlet_text);
    tell({0x31bf1, 0x10690ee0}, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
}

// Story 0x5c: a club is touted as a winner of its cup competition, with a second paragraph on the match that
// confirmed it (appended to the body).
void Story::cup_favourites() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) media_on_competition({{0x326ab, 0x106918c8}, {0x325e1, 0x10691844}}, Form::plain);
        else foreign_media_on_competition({0x106918ec, 0x1c}, {{0x3281a, 0x10691a28}, {0x3272e, 0x1069198c}}, Order::club_competition_nation, Form::plain);
    } else if (cmp(kind, 1) >= 0 || cmp(kind, 0) < 0) {
        // The outlet that carries the story is named; r13 holds the club table.
        const auto w = pick({{0x32a30, 0x10691ca0}, {0x328bf, 0x10691b68}});
        outgoing(0, home_marker());
        outgoing(1, home_marker());
        outgoing(2, nation_marker());
        outgoing(3, nation_marker());
        scratch<13>(clubs());
        outgoing(4, home_short());
        outgoing(5, competition_long_name());
        outgoing(6, outlet_text);
        outgoing(7, home_short());
        outgoing(8, nation_name());
        tell(w, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
    } else if (subject_has_club_and_job()) {
        staff_on_competition({0x106911d8, 0x74}, {{0x320fe, 0x106913b8}, {0x32029, 0x106912cc}}, Form::plain);
    } else if (subject_is_former_player()) {
        former_player_on_competition({{0x32289, 0x106914e8}, {0x321d0, 0x1069145c}}, Form::plain);
    } else if (at_home()) {
        media_on_competition({{0x323db, 0x10691600}, {0x3233f, 0x1069157c}}, Form::plain);
    } else {
        media_in_nation_on_competition({{0x3252c, 0x106917b0}, {0x32474, 0x106916dc}}, Form::plain);
    }

    // The second paragraph: the match that confirmed the tip, when the club won it.
    if (cmp(sb(has_match), 0) == 0 || cmp(home_goals(), away_goals()) <= 0) return;
    score(0x10691d20);
    const auto say_quote = [&](Wording w, std::initializer_list<std::uint32_t> args) { say(quote, phrase(w.phrase), w.back, args); };
    if (cmp(subject_id, -1) <= 0 || cmp(kind, 0) != 0) {
        const auto variant = rem3(kind + club_id);
        if (cmp(variant, 1) == 0) {
            const auto w = by_margin({{{0x32ef3, 0x1069208c}, {0x3300a, 0x10692160}, {0x32f73, 0x10692104}}});
            say_quote(w, {rival_long_marker(), rival_long_marker(), rival_long(), score_text, clubs(), clubs()});
        } else if (cmp(variant, 1) >= 0 || cmp(variant, 0) < 0) {
            const auto w = by_margin({{{0x3308a, 0x106921d8}, {0x3318c, 0x106922ac}, {0x3310d, 0x10692250}}});
            say_quote(w, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
        } else {
            const auto w = by_margin({{{0x32d3a, 0x10691f40}, {0x32e67, 0x10692014}, {0x32dd1, 0x10691fb8}}});
            say_quote(w, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
        }
    } else {
        const auto w = by_margin({{{0x32b97, 0x10691dac}, {0x32cb6, 0x10691e90}, {0x32c1b, 0x10691e2c}}});
        say_quote(w, {unmarked, rival_long_marker(), rival_long_marker(), subject_short, rival_long(), score_text});
    }
    call(append_entry, 0x106922b8, {{3, body}, {4, quote}});
}

// Story 0x5b: a club spends big and the media expect success.
void Story::big_spenders() {
    put32(at(0x88), spent);
    call(entry::to_amount, 0x106922d8, {{3, at(0x88)}, {4, 6}});
    const auto amount_high = c.r(3), amount_low = c.r(4);  // (the converted amount, in two words)
    call(entry::format_amount, 0x106922f0, {{3, amount_text}, {4, 0x32}, {5, amount_high}, {6, amount_low}, {7, 6}});
    if (cmp(kind, 1) == 0) {
        if (cmp(subject_id, -1) <= 0) return tell({0x3363e, 0x10692610}, {home_marker(), home_marker(), home_short(), amount_text, clubs()});
        return tell({0x33540, 0x106925c0}, {home_marker(), home_marker(), home_short(), manager_full, amount_text, subject_full});
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        const auto competition_told = [&](Wording w, bool signing) {
            if (signing) {
                outgoing(0, home_short());
                outgoing(1, competition_long_name());
                return tell(w, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), amount_text, subject_full});
            }
            outgoing(0, competition_long_name());
            tell(w, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), amount_text, home_short()});
        };
        if (cmp(title_holder, 0) == 0) {
            if (cmp(subject_id, -1) <= 0) return competition_told({0x33484, 0x10692560}, false);
            return competition_told({0x3339e, 0x106924d4}, true);
        }
        if (cmp(subject_id, -1) <= 0) return competition_told({0x332e9, 0x10692438}, false);
        return competition_told({0x33201, 0x106923ac}, true);
    }
    if (at_home()) {
        if (cmp(subject_id, -1) <= 0) return tell({0x337ac, 0x106926c4}, {home_marker(), home_marker(), home_short(), amount_text, clubs()});
        return tell({0x336d1, 0x10692674}, {home_marker(), home_marker(), home_short(), amount_text, subject_full});
    }
    adjective_of_nation({0x106926e8, 0x1c});  // (the adjective is built and not used)
    if (cmp(subject_id, -1) <= 0) {
        outgoing(0, amount_text);
        return tell({0x3395c, 0x10692824}, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation_name(), home_short()});
    }
    outgoing(0, amount_text);
    outgoing(1, subject_full);
    tell({0x33861, 0x1069278c}, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation_name(), home_short()});
}

// Story 0x52: the club's midfield is accused of starving its striker (the subject) of chances.
void Story::starved_striker() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return tell({0x33ba7, 0x1069297c}, {home_marker(), home_marker(), home_short(), subject_full, subject_short});
        outgoing(0, subject_full);
        outgoing(1, subject_short);
        return tell({0x33cca, 0x10692a1c}, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation_name(), home_short()});
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (at_home()) return tell({0x33a2d, 0x106928a0}, {home_marker(), home_marker(), home_short(), subject_full, clubs()});
        adjective_of_nation({0x106928c4, 0x1c});
        outgoing(0, subject_full);
        return tell({0x33ad9, 0x10692920}, {home_marker(), home_marker(), unmarked, unmarked, home_short(), nationality});
    }
    if (at_home()) return tell({0x33e09, 0x10692a78}, {home_marker(), home_marker(), subject_full, home_short(), subject_short});
    adjective_of_nation({0x10692a9c, 0x1e});
    outgoing(0, nationality);
    outgoing(1, subject_short);
    tell({0x33f75, 0x10692b00}, {home_marker(), home_marker(), unmarked, unmarked, subject_full, home_short()});
}

// Story 0x3e: a club deep in the relegation zone.
void Story::relegation_mire() {
    if (cmp(kind, 1) == 0) {
        if (at_home()) return media_on_competition({0x3432f, 0x10692cf8}, Form::plain);
        return foreign_media_on_competition({0x10692d1c, 0x1f}, {0x34407, 0x10692db4}, Order::nation_club_competition, Form::plain);
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (at_home()) return media_on_competition({0x34103, 0x10692bb0}, Form::plain);
        return foreign_media_on_competition({0x10692bd4, 0x1c}, {0x34208, 0x10692c6c}, Order::club_competition_nation, Form::plain);
    }
    media_on_competition({0x34501, 0x10692e38}, Form::plain);
}

// Story 0x5e: the press run out of superlatives for a winning club.
void Story::winning_form_praised() {
    if (cmp(kind, 2) == 0) {
        if (at_home()) return media_on_competition({0x34bf7, 0x10693294}, Form::long_form);
        outgoing(0, home_short());
        outgoing(1, competition_long_name());
        outgoing(2, nation_name());
        return tell({0x34cbe, 0x10693368}, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), nation_marker(), nation_marker()});
    }
    if (cmp(kind, 2) < 0) {
        if (cmp(kind, 0) != 0) {
            if (cmp(kind, 0) >= 0) {
                // Result 1: the supporters' hopes, with the manager on the radio.
                const auto w = pick({{0x34ab8, 0x10693208}, {0x3496f, 0x10693174}});
                outgoing(0, competition_long_name());
                outgoing(1, manager_full);
                return tell(w, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), unmarked, home_short()});
            }
        } else {
            if (at_home()) return media_on_competition({{0x346cf, 0x10692f78}, {0x345f6, 0x10692ef4}}, Form::long_form);
            return foreign_media_on_competition({0x10692f9c, 0x1c}, {{0x3488a, 0x106930d8}, {0x34792, 0x1069303c}}, Order::nation_club_competition, Form::long_form);
        }
    }
    if (at_home()) return media_on_club({{0x34e37, 0x10693410}, {0x34d97, 0x106933c4}});
    adjective_of_nation({0x10693434, 0x1c});
    const auto w = pick({{0x34f97, 0x106934e8}, {0x34ed1, 0x10693490}});
    tell(w, {home_marker(), home_marker(), unmarked, unmarked, home_short(), nationality});
}

// Story 0x3c: supporters losing patience.
void Story::fans_losing_patience() {
    if (cmp(kind, 1) == 0) return media_on_club({0x350fb, 0x106935a0});
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) return media_on_club({0x35053, 0x10693554});
    if (at_home()) return media_on_club({0x35222, 0x106935f4});
    adjective_of_nation({0x10693618, 0x17});
    tell({0x3531e, 0x1069366c}, {home_marker(), home_marker(), unmarked, unmarked, home_short(), nationality});
}
}
