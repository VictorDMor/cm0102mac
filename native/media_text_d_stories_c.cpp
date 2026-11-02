// The supporters' protest after a match.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

// Story 0x3d: the supporters turn on the club after a match. The wording depends on the kind (2: the user's own
// view, 0 and 1: the club's fans, other: unhappy), on whether the match was a defeat or a draw, and on whether
// the match has a named knockout round in its competition.
void Story::fans_protest_match() {
    if (cmp(sb(has_match), 0) == 0) return no_match();
    score(0x106936f0);
    const auto decisive = [&] { return cmp(home_goals(), away_goals()) != 0; };

    if (cmp(i16(at(0xd8)), 0) == 0 && cmp(i16(at(0xda)), 0) == 0) {
        // No date for the match, so no round to name.
        if (cmp(kind, 2) == 0) {
            if (decisive()) {
                outgoing(0, home_short());
                outgoing(1, score_text);
                outgoing(2, rival_long());
                outgoing(3, outlet_text);
                return tell({0x36032, 0x10694c94}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            }
            outgoing(0, home_short());
            outgoing(1, score_text);
            outgoing(2, rival_long());
            outgoing(3, outlet_text);
            tell({0x35f59, 0x10694a70}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            return level_ending(0x10694abc);
        }
        if (cmp(kind, 2) < 0) {
            if (cmp(kind, 0) == 0) {
                if (decisive()) {
                    outgoing(0, rival_long());
                    outgoing(1, home_short());
                    return tell({0x35cb3, 0x106946ac}, {unmarked, rival_long_marker(), rival_long_marker(), home_marker(), home_marker(), score_text});
                }
                outgoing(0, rival_long());
                outgoing(1, home_short());
                tell({0x35bf0, 0x106944a0}, {unmarked, rival_long_marker(), rival_long_marker(), home_marker(), home_marker(), score_text});
                return level_ending(0x106944ec);
            }
            if (cmp(kind, 0) >= 0) {
                if (decisive()) {
                    outgoing(0, outlet_text);
                    outgoing(1, home_short());
                    outgoing(2, score_text);
                    outgoing(3, rival_long());
                    return tell({0x35e66, 0x106949a0}, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), rival_long_marker(), rival_long_marker()});
                }
                outgoing(0, outlet_text);
                outgoing(1, home_short());
                outgoing(2, score_text);
                outgoing(3, rival_long());
                tell({0x35d80, 0x1069477c}, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), rival_long_marker(), rival_long_marker()});
                return level_ending(0x106947c8);
            }
        }
        // Any other kind: the plain protest.
        if (decisive()) {
            outgoing(0, rival_long());
            return tell({0x361d0, 0x10694f48}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), score_text});
        }
        outgoing(0, rival_long());
        tell({0x3610e, 0x10694d44}, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), score_text});
        return level_ending(0x10694d90);
    }

    // The round's name, from the match's competition and date. A friendly or a reserve match starts its label in
    // lower case; the label's marker (r3 of the article routine) goes with it into the phrases.
    call(stage_label, 0x10693730, {{3, competitions() + (u32(at(0xd4)) * competition_size)}, {4, i16(at(0xd8))}, {5, i16(at(0xda))}, {6, 1}, {7, stage_name}, {8, 0xc8}});
    call(compare_entry, 0x10693740, {{3, stage_name}, {4, phrase(0x247bc)}});
    if (cmp(c.r(3), 0) == 0) {  // (the compare routine's result: 0 when equal)
        put8(at(0x304), 0x66);
    } else {
        call(compare_entry, 0x10693764, {{3, stage_name}, {4, phrase(0x247c5)}});
        if (cmp(c.r(3), 0) == 0) put8(at(0x304), 0x72);
    }
    call(stage_ordinal, 0x10693788, {{3, 0}, {4, i16(at(0xd8))}, {5, i16(at(0xda))}, {6, 1}});
    const auto stage_marker = c.r(3);

    // The unhappy-fans wordings, for a kind above 2 or below 0.
    const auto unhappy = [&] {
        if (decisive()) {
            outgoing(0, home_short());
            outgoing(1, score_text);
            outgoing(2, stage_name);
            outgoing(3, rival_long());
            return tell({0x35b07, 0x106943c8}, {home_marker(), home_marker(), sb(stage_marker), sb(stage_marker), rival_long_marker(), rival_long_marker()});
        }
        outgoing(0, home_short());
        outgoing(1, score_text);
        outgoing(2, stage_name);
        outgoing(3, rival_long());
        tell({0x35a28, 0x106941ac}, {home_marker(), home_marker(), sb(stage_marker), sb(stage_marker), rival_long_marker(), rival_long_marker()});
        level_ending(0x106941f8);
    };
    if (cmp(kind, 2) == 0) {
        // Furious fans: the club table is in r13.
        outgoing(0, sb(outlet_flag()));
        outgoing(1, sb(outlet_flag()));
        const bool beaten = decisive();
        scratch<13>(clubs());
        outgoing(2, home_short());
        outgoing(3, score_text);
        outgoing(4, stage_name);
        outgoing(5, rival_long());
        outgoing(6, outlet_text);
        tell(beaten ? Wording{0x35921, 0x106940e4} : Wording{0x35827, 0x10693ea8},
             {home_marker(), home_marker(), sb(stage_marker), sb(stage_marker), rival_long_marker(), rival_long_marker()});
        if (!beaten) level_ending(0x10693ef4);
        return;
    }
    if (cmp(kind, 2) >= 0) return unhappy();
    if (cmp(kind, 0) == 0) {
        // Boos and jeers late in the match.
        const bool beaten = decisive();
        outgoing(0, home_marker());
        outgoing(1, score_text);
        outgoing(2, stage_name);
        outgoing(3, rival_long());
        outgoing(4, home_short());
        tell(beaten ? Wording{0x3551e, 0x10693a9c} : Wording{0x3543a, 0x10693878},
             {unmarked, sb(stage_marker), sb(stage_marker), rival_long_marker(), rival_long_marker(), home_marker()});
        if (!beaten) level_ending(0x106938c4);
        return;
    }
    if (cmp(kind, 0) < 0) return unhappy();
    // Kind 1: a magazine reports the demonstration; r13 holds the rival's row and r14 the club table.
    const bool beaten = decisive();
    outgoing(0, rival_long_marker());
    scratch<14>(clubs());
    const auto rival_row = scratch<13>(rival());
    outgoing(1, club_view(rival_row).long_marker());
    outgoing(2, outlet_text);
    outgoing(3, home_short());
    outgoing(4, score_text);
    outgoing(5, stage_name);
    outgoing(6, rival_long());
    tell(beaten ? Wording{0x35713, 0x10693dc0} : Wording{0x3560c, 0x10693b84},
         {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(stage_marker), sb(stage_marker)});
    if (!beaten) level_ending(0x10693bd0);
}
}
