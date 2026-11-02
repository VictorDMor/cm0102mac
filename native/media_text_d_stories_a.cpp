// The match stories: runs, wins and defeats, form, and the results that end them.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

void Story::story_50() {
    // Story code 0x50: opens with an unbeaten run of games, the score and the opposition.
    if (cmp(sb(has_match), 0) == 0) {
        put8(toc(-0x7bb8), 0);
        return;
    }
    score(0x1068ad1c);
    if (cmp(kind, 1) != 0) {
        if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
            if (cmp(staff_id, -1) <= 0) {
                if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                    outgoing(0, score_text);
                    outgoing(1, rival_long());
                    say(body, phrase(0x2b979), 0x1068b030, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
                    return;
                }
                if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                    outgoing(0, score_text);
                    outgoing(1, rival_long());
                    say(body, phrase(0x2baf4), 0x1068b18c, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
                    return;
                }
                outgoing(0, score_text);
                outgoing(1, rival_long());
                say(body, phrase(0x2ba2e), 0x1068b0ec, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
                return;
            }
            if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                outgoing(0, c.r(14));
                outgoing(1, score_text);
                outgoing(2, subject_full);
                outgoing(3, rival_long());
                say(body, phrase(0x2b6b9), 0x1068ae00, {home_marker(), home_marker(), 0x64, rival_long_marker(), rival_long_marker(), home_short()});
                return;
            }
            if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                outgoing(0, c.r(14));
                outgoing(1, score_text);
                outgoing(2, subject_full);
                outgoing(3, rival_long());
                say(body, phrase(0x2b896), 0x1068af74, {home_marker(), home_marker(), 0x64, rival_long_marker(), rival_long_marker(), home_short()});
                return;
            }
            outgoing(0, c.r(14));
            outgoing(1, score_text);
            outgoing(2, subject_full);
            outgoing(3, rival_long());
            say(body, phrase(0x2b79e), 0x1068aec8, {home_marker(), home_marker(), 0x64, rival_long_marker(), rival_long_marker(), home_short()});
            return;
        }
    } else {
        if (cmp(staff_id, -1) <= 0) {
            if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                outgoing(0, rival_long());
                outgoing(1, score_text);
                say(body, phrase(0x2bf3c), 0x1068b564, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), c.r(14), home_short()});
                return;
            }
            if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                outgoing(0, rival_long());
                outgoing(1, score_text);
                say(body, phrase(0x2c0ae), 0x1068b6c0, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), c.r(14), home_short()});
                return;
            }
            outgoing(0, rival_long());
            outgoing(1, score_text);
            say(body, phrase(0x2bfee), 0x1068b620, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), c.r(14), home_short()});
            return;
        }
        if (cmp((home_goals()) - (away_goals()), 3) > 0) {
            outgoing(0, c.r(14));
            outgoing(1, home_short());
            outgoing(2, rival_long());
            outgoing(3, score_text);
            outgoing(4, rival_long());
            outgoing(5, subject_full);
            c.set(13, home());
            say(body, phrase(0x2bba7), 0x1068b2a4, {sb(u8(c.r(13) + 0x23b)), home_marker(), rival_long_marker(), rival_long_marker(), rival_marker(), rival_marker()});
            return;
        }
        if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
            outgoing(0, c.r(14));
            outgoing(1, home_short());
            outgoing(2, rival_long());
            outgoing(3, score_text);
            outgoing(4, rival_long());
            outgoing(5, subject_full);
            c.set(13, home());
            say(body, phrase(0x2be0f), 0x1068b4a8, {sb(u8(c.r(13) + 0x23b)), home_marker(), rival_long_marker(), rival_long_marker(), rival_marker(), rival_marker()});
            return;
        }
        outgoing(0, c.r(14));
        outgoing(1, home_short());
        outgoing(2, rival_long());
        outgoing(3, score_text);
        outgoing(4, rival_long());
        outgoing(5, subject_full);
        c.set(13, home());
        say(body, phrase(0x2bcd4), 0x1068b3b4, {sb(u8(c.r(13) + 0x23b)), home_marker(), rival_long_marker(), rival_long_marker(), rival_marker(), rival_marker()});
        return;
    }
    if (cmp(staff_id, -1) <= 0) {
        if (cmp((home_goals()) - (away_goals()), 3) > 0) {
            outgoing(0, score_text);
            outgoing(1, rival_long());
            say(body, phrase(0x2c48a), 0x1068b9b4, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
            return;
        }
        if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
            outgoing(0, score_text);
            outgoing(1, rival_long());
            say(body, phrase(0x2c61f), 0x1068bb10, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
            return;
        }
        outgoing(0, score_text);
        outgoing(1, rival_long());
        say(body, phrase(0x2c555), 0x1068ba70, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
        return;
    }
    if (cmp((home_goals()) - (away_goals()), 3) > 0) {
        outgoing(0, score_text);
        outgoing(1, rival_long());
        outgoing(2, subject_full);
        say(body, phrase(0x2c160), 0x1068b78c, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
        return;
    }
    if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
        outgoing(0, score_text);
        outgoing(1, rival_long());
        outgoing(2, subject_full);
        say(body, phrase(0x2c37c), 0x1068b8f8, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
        return;
    }
    outgoing(0, score_text);
    outgoing(1, rival_long());
    outgoing(2, subject_full);
    say(body, phrase(0x2c264), 0x1068b850, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
}

void Story::story_51() {
    // Story code 0x51: opens with a run of results, the opposition and the score.
    if (cmp(sb(has_match), 0) == 0) {
        put8(toc(-0x7bb8), 0);
        return;
    }
    score(0x1068bb94);
    if (cmp(kind, 1) == 0) {
        if (cmp(staff_id, -1) <= 0) {
            outgoing(0, score_text);
            outgoing(1, rival_long());
            say(body, phrase(0x2c98e), 0x1068be44, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
            return;
        }
        outgoing(0, score_text);
        outgoing(1, rival_long());
        outgoing(2, c.r(14));
        say(body, phrase(0x2c8b2), 0x1068bda4, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), subject_full, home_short()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0) {
            outgoing(0, rival_long());
            outgoing(1, score_text);
            say(body, phrase(0x2c805), 0x1068bcf8, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
            return;
        }
        outgoing(0, rival_long());
        outgoing(1, score_text);
        outgoing(2, subject_full);
        say(body, phrase(0x2c6df), 0x1068bc58, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), c.r(14)});
        return;
    }
    if (cmp(staff_id, -1) <= 0) {
        outgoing(0, score_text);
        outgoing(1, c.r(14));
        c.set(13, rival_id() * 0x248);
        c.set(14, clubs());
        say(body, phrase(0x2cb30), 0x1068bf8c, {home_marker(), home_marker(), sb(u8((c.r(14) + c.r(13)) + 0x242)), rival_long_marker(), home_short(), rival_short()});
        return;
    }
    outgoing(0, score_text);
    outgoing(1, c.r(14));
    outgoing(2, subject_full);
    c.set(13, clubs());
    say(body, phrase(0x2ca3b), 0x1068bef0, {home_marker(), home_marker(), rival_long_marker(), sb(u8((c.r(13) + (rival_id() * 0x248)) + 0x242)), home_short(), rival_short()});
}

void Story::story_3f() {
    // Story code 0x3f: a club in a relegation battle.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x2cf9b), 0x1068c33c, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            return;
        }
        call(0x10168a00, 0x1068c360, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        outgoing(0, home_short());
        outgoing(1, competition_name());
        outgoing(2, nationality);
        say(body, phrase(0x2d0ab), 0x1068c3f8, {home_marker(), home_marker(), competition_marker(), competition_marker(), 0x64, 0x64});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                if (cmp(sb(c.r(16)), 0) != 0) {
                    say(body, phrase(0x2cdaf), 0x1068c1f4, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                    return;
                }
                call(0x10168a00, 0x1068c218, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
                outgoing(0, home_short());
                outgoing(1, nationality);
                outgoing(2, competition_name());
                say(body, phrase(0x2ce94), 0x1068c2b0, {home_marker(), home_marker(), 0x64, 0x64, competition_marker(), competition_marker()});
                return;
            } else {
                outgoing(0, competition_name());
                say(body, phrase(0x2ccd3), 0x1068c168, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                return;
            }
        } else {
            call(0x100c6bf0, 0x1068bfe0, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x85)}});
            c.set(0, c.r(3));
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(3));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x2cbd1), 0x1068c0cc, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x85))), sb(u8(at(0x85))), home_marker(), home_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) == 0) {
        say(body, phrase(0x2d297), 0x1068c4dc, {home_marker(), home_marker(), nation_marker(), nation_marker(), home_short(), nation() + 0xe1});
        return;
    }
    say(body, phrase(0x2d1dd), 0x1068c44c, {home_marker(), home_marker(), home_short(), clubs()});
}

void Story::story_5f() {
    // Story code 0x5f: a club tipped for promotion.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x2d6a2), 0x1068c828, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            return;
        }
        outgoing(0, nation() + 0xe1);
        outgoing(1, home_short());
        outgoing(2, competition_name());
        say(body, phrase(0x2d75d), 0x1068c8fc, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_marker(), competition_marker()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                if (cmp(sb(c.r(16)), 0) == 0) {
                    say(body, phrase(0x2d5d1), 0x1068c79c, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
                    return;
                }
                say(body, phrase(0x2d51c), 0x1068c70c, {home_marker(), home_marker(), home_short(), clubs()});
                return;
            } else {
                outgoing(0, competition_name());
                say(body, phrase(0x2d452), 0x1068c6b8, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                return;
            }
        } else {
            call(0x100c6bf0, 0x1068c530, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x84)}});
            c.set(0, c.r(3));
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(3));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x2d363), 0x1068c61c, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x84))), sb(u8(at(0x84))), home_marker(), home_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        say(body, phrase(0x2d834), 0x1068c988, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    call(0x10168a00, 0x1068c9ac, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    outgoing(0, nationality);
    outgoing(1, home_short());
    outgoing(2, competition_long_name());
    say(body, phrase(0x2d914), 0x1068ca44, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
}

void Story::story_60() {
    // Story code 0x60: a club climbing to safety.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x2dd4c), 0x1068cd90, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            return;
        }
        outgoing(0, nation() + 0xe1);
        outgoing(1, home_short());
        outgoing(2, competition_name());
        say(body, phrase(0x2de0b), 0x1068ce64, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_marker(), competition_marker()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                if (cmp(sb(c.r(16)), 0) == 0) {
                    say(body, phrase(0x2dc94), 0x1068cd04, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
                    return;
                }
                say(body, phrase(0x2dbf8), 0x1068cc74, {home_marker(), home_marker(), home_short(), clubs()});
                return;
            } else {
                outgoing(0, competition_name());
                say(body, phrase(0x2db1a), 0x1068cc20, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                return;
            }
        } else {
            call(0x100c6bf0, 0x1068ca98, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x83)}});
            c.set(0, c.r(3));
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(3));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x2da16), 0x1068cb84, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x83))), sb(u8(at(0x83))), home_marker(), home_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        say(body, phrase(0x2dee6), 0x1068cef0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    call(0x10168a00, 0x1068cf14, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    outgoing(0, nationality);
    outgoing(1, home_short());
    outgoing(2, competition_long_name());
    say(body, phrase(0x2dfca), 0x1068cfac, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
}

void Story::story_42() {
    // Story code 0x42: the press write a club off.
    if (cmp(kind, 0) != 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            if (cmp(field15, 0) == 0) {
                say(body, phrase(0x2e6fc), 0x1068d5fc, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                return;
            }
            say(body, phrase(0x2e63a), 0x1068d578, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        call(0x10168a00, 0x1068d620, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        if (cmp(field15, 0) == 0) {
            outgoing(0, home_short());
            outgoing(1, competition_long_name());
            outgoing(2, nationality);
            say(body, phrase(0x2e88f), 0x1068d75c, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), 0x64, 0x64});
            return;
        }
        outgoing(0, home_short());
        outgoing(1, competition_long_name());
        outgoing(2, nationality);
        say(body, phrase(0x2e7ab), 0x1068d6c0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), 0x64, 0x64});
        return;
    }
    if (cmp(staff_id, -1) > 0 && cmp(field19, -1) > 0 && cmp(field1a, -1) > 0) {
        call(0x100c6bf0, 0x1068cff0, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x82)}});
        c.set(0, c.r(3));
        if (cmp(field15, 0) == 0) {
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(0));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x2e1ba), 0x1068d1d0, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x82))), sb(u8(at(0x82))), home_marker(), home_marker()});
            return;
        }
        outgoing(0, competition_marker());
        outgoing(1, competition_marker());
        outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
        outgoing(3, c.r(3));
        outgoing(4, subject_full);
        outgoing(5, home_short());
        outgoing(6, competition_name());
        say(body, phrase(0x2e0d0), 0x1068d0e4, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x82))), sb(u8(at(0x82))), home_marker(), home_marker()});
        return;
    }
    if (cmp(staff_id, -1) > 0 && cmp(sb(c.r(13)), 0) != 0) {
        outgoing(0, competition_name());
        say(body, phrase(0x2e29f), 0x1068d26c, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
        return;
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x2e3fe), 0x1068d384, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        say(body, phrase(0x2e354), 0x1068d300, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    call(0x10168a00, 0x1068d3a8, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1b}});
    if (cmp(field15, 0) == 0) {
        outgoing(0, nationality);
        outgoing(1, home_short());
        outgoing(2, competition_long_name());
        say(body, phrase(0x2e571), 0x1068d4e4, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
        return;
    }
    outgoing(0, nationality);
    outgoing(1, home_short());
    outgoing(2, competition_long_name());
    say(body, phrase(0x2e4ae), 0x1068d448, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
}

void Story::story_43() {
    // Story code 0x43: a club the press find boring.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x2ec87), 0x1068daa8, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        call(0x10168a00, 0x1068dacc, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        outgoing(0, home_short());
        outgoing(1, nationality);
        outgoing(2, competition_long_name());
        say(body, phrase(0x2ed5e), 0x1068db64, {home_marker(), home_marker(), 0x64, 0x64, competition_long_marker(), competition_long_marker()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                if (cmp(sb(c.r(16)), 0) != 0) {
                    say(body, phrase(0x2eb27), 0x1068d994, {home_marker(), home_marker(), 0x64, home_short(), other_full});
                    return;
                }
                call(0x10168a00, 0x1068d9b8, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
                outgoing(0, nationality);
                outgoing(1, other_full);
                say(body, phrase(0x2ebc7), 0x1068da1c, {home_marker(), home_marker(), 0x64, 0x64, 0x64, home_short()});
                return;
            } else {
                outgoing(0, competition_name());
                say(body, phrase(0x2ea60), 0x1068d938, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                return;
            }
        } else {
            call(0x100c6bf0, 0x1068d7b0, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x81)}});
            c.set(0, c.r(3));
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(3));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x2e960), 0x1068d89c, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x81))), sb(u8(at(0x81))), home_marker(), home_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        say(body, phrase(0x2ee55), 0x1068dbb8, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    call(0x10168a00, 0x1068dbdc, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    say(body, phrase(0x2ef2b), 0x1068dc68, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
}

void Story::story_44() {
    // Story code 0x44: the press do not see a club as a contender.
    if (cmp(kind, 0) != 0) {
        if (cmp(field15, 0) == 0) {
            if (cmp(field16, u32(nation() + 0x68)) != 0 && cmp(field17, u32(nation() + 0x68)) != 0) {
                if (cmp(field17, -1) <= 0) {
                    outgoing(0, home_short());
                    outgoing(1, competition_long_name());
                    outgoing(2, persons() + ((field16 * 0x128) + 0xe1));
                    say(body, phrase(0x303dc), 0x1068f46c, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                    return;
                }
                outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                outgoing(2, home_short());
                outgoing(3, competition_long_name());
                outgoing(4, persons() + ((field16 * 0x128) + 0xe1));
                outgoing(5, persons() + ((field17 * 0x128) + 0xe1));
                c.set(13, home());
                say(body, phrase(0x302ee), 0x1068f3a4, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                return;
            }
        } else {
            if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                say(body, phrase(0x2ffb8), 0x1068eff8, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                return;
            } else {
                if (cmp(field17, -1) <= 0) {
                    outgoing(0, home_short());
                    outgoing(1, competition_long_name());
                    outgoing(2, persons() + ((field16 * 0x128) + 0xe1));
                    say(body, phrase(0x30173), 0x1068f1d4, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                    return;
                }
                outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                outgoing(2, home_short());
                outgoing(3, competition_long_name());
                outgoing(4, persons() + ((field16 * 0x128) + 0xe1));
                outgoing(5, persons() + ((field17 * 0x128) + 0xe1));
                c.set(13, home());
                say(body, phrase(0x30072), 0x1068f10c, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                return;
            }
        }
    } else {
        if (cmp(field15, 0) == 0) {
            if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                    if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                        say(body, phrase(0x2f97e), 0x1068e850, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                        return;
                    } else {
                        outgoing(0, competition_long_name());
                        say(body, phrase(0x2f8c0), 0x1068e7cc, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), subject_full, home_short()});
                        return;
                    }
                } else {
                    call(0x100c6bf0, 0x1068e644, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x7d)}});
                    c.set(0, c.r(3));
                    outgoing(0, competition_long_marker());
                    outgoing(1, competition_long_marker());
                    outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                    outgoing(3, c.r(3));
                    outgoing(4, subject_full);
                    outgoing(5, home_short());
                    outgoing(6, competition_long_name());
                    say(body, phrase(0x2f7e0), 0x1068e730, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x7d))), sb(u8(at(0x7d))), home_marker(), home_marker()});
                    return;
                }
            } else {
                if (cmp(field17, -1) <= 0) {
                    if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                            outgoing(0, home_short());
                            outgoing(1, competition_long_name());
                            outgoing(2, persons() + ((field16 * 0x128) + 0xe1));
                            say(body, phrase(0x2fef5), 0x1068ef34, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        } else {
                            outgoing(0, subject_full);
                            outgoing(1, home_short());
                            outgoing(2, competition_long_name());
                            outgoing(3, persons() + ((field16 * 0x128) + 0xe1));
                            say(body, phrase(0x2fe21), 0x1068ee6c, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        }
                    } else {
                        call(0x100c6bf0, 0x1068ec5c, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x7b)}});
                        c.set(0, c.r(3));
                        outgoing(0, competition_long_marker());
                        c.set(13, (field19 * 0x248) + 0x221);
                        outgoing(1, competition_long_marker());
                        outgoing(2, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        outgoing(3, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        c.set(14, clubs());
                        c.set(13, c.r(14) + c.r(13));
                        outgoing(4, c.r(13));
                        outgoing(5, c.r(3));
                        outgoing(6, subject_full);
                        outgoing(7, home_short());
                        outgoing(8, competition_long_name());
                        outgoing(9, persons() + ((field16 * 0x128) + 0xe1));
                        say(body, phrase(0x2fd2b), 0x1068ed8c, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x7b))), sb(u8(at(0x7b))), home_marker(), home_marker()});
                        return;
                    }
                } else {
                    if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                            outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(2, home_short());
                            outgoing(3, competition_long_name());
                            outgoing(4, persons() + ((field16 * 0x128) + 0xe1));
                            outgoing(5, persons() + ((field17 * 0x128) + 0xe1));
                            c.set(13, home());
                            say(body, phrase(0x2fc3f), 0x1068ec2c, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        } else {
                            outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(2, subject_full);
                            outgoing(3, home_short());
                            outgoing(4, competition_long_name());
                            outgoing(5, persons() + ((field16 * 0x128) + 0xe1));
                            outgoing(6, persons() + ((field17 * 0x128) + 0xe1));
                            c.set(13, home());
                            say(body, phrase(0x2fb42), 0x1068eb20, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        }
                    } else {
                        call(0x100c6bf0, 0x1068e888, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x7c)}});
                        c.set(0, c.r(3));
                        outgoing(0, competition_long_marker());
                        outgoing(1, competition_long_marker());
                        c.set(14, (competition_id * 0x6c) + 0x1a);
                        outgoing(2, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        c.set(13, (field16 * 0x128) + 0xe1);
                        outgoing(3, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        outgoing(4, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                        c.set(16, persons());
                        c.set(16, u8((c.r(16) + (field17 * 0x128)) + 0x116));
                        c.set(16, sb(c.r(16)));
                        outgoing(5, c.r(16));
                        c.set(16, clubs());
                        outgoing(6, c.r(16) + ((field19 * 0x248) + 0x221));
                        outgoing(7, c.r(3));
                        outgoing(8, subject_full);
                        outgoing(9, home_short());
                        outgoing(10, competitions() + c.r(14));
                        outgoing(11, persons() + c.r(13));
                        outgoing(12, persons() + ((field17 * 0x128) + 0xe1));
                        say(body, phrase(0x2fa2b), 0x1068e9fc, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x7c))), sb(u8(at(0x7c))), home_marker(), home_marker()});
                        return;
                    }
                }
            }
        } else {
            if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                    if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                        say(body, phrase(0x2f1b8), 0x1068def8, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                        return;
                    } else {
                        outgoing(0, competition_long_name());
                        say(body, phrase(0x2f0f8), 0x1068de74, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), subject_full, home_short()});
                        return;
                    }
                } else {
                    call(0x100c6bf0, 0x1068dcec, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x80)}});
                    c.set(0, c.r(3));
                    outgoing(0, competition_long_marker());
                    outgoing(1, competition_long_marker());
                    outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                    outgoing(3, c.r(3));
                    outgoing(4, subject_full);
                    outgoing(5, home_short());
                    outgoing(6, competition_long_name());
                    say(body, phrase(0x2f01e), 0x1068ddd8, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x80))), sb(u8(at(0x80))), home_marker(), home_marker()});
                    return;
                }
            } else {
                if (cmp(field17, -1) <= 0) {
                    if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                            outgoing(0, home_short());
                            outgoing(1, competition_long_name());
                            outgoing(2, persons() + ((field16 * 0x128) + 0xe1));
                            say(body, phrase(0x2f723), 0x1068e5dc, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        } else {
                            outgoing(0, subject_full);
                            outgoing(1, home_short());
                            outgoing(2, competition_long_name());
                            outgoing(3, persons() + ((field16 * 0x128) + 0xe1));
                            say(body, phrase(0x2f64d), 0x1068e514, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        }
                    } else {
                        call(0x100c6bf0, 0x1068e304, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x7e)}});
                        c.set(0, c.r(3));
                        outgoing(0, competition_long_marker());
                        c.set(13, (field19 * 0x248) + 0x221);
                        outgoing(1, competition_long_marker());
                        outgoing(2, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        outgoing(3, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        c.set(14, clubs());
                        c.set(13, c.r(14) + c.r(13));
                        outgoing(4, c.r(13));
                        outgoing(5, c.r(3));
                        outgoing(6, subject_full);
                        outgoing(7, home_short());
                        outgoing(8, competition_long_name());
                        outgoing(9, persons() + ((field16 * 0x128) + 0xe1));
                        say(body, phrase(0x2f55d), 0x1068e434, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x7e))), sb(u8(at(0x7e))), home_marker(), home_marker()});
                        return;
                    }
                } else {
                    if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                            outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(2, home_short());
                            outgoing(3, competition_long_name());
                            outgoing(4, persons() + ((field16 * 0x128) + 0xe1));
                            outgoing(5, persons() + ((field17 * 0x128) + 0xe1));
                            c.set(13, home());
                            say(body, phrase(0x2f477), 0x1068e2d4, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        } else {
                            outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                            outgoing(2, subject_full);
                            outgoing(3, home_short());
                            outgoing(4, competition_long_name());
                            outgoing(5, persons() + ((field16 * 0x128) + 0xe1));
                            outgoing(6, persons() + ((field17 * 0x128) + 0xe1));
                            c.set(13, home());
                            say(body, phrase(0x2f378), 0x1068e1c8, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                            return;
                        }
                    } else {
                        call(0x100c6bf0, 0x1068df30, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x7f)}});
                        c.set(0, c.r(3));
                        outgoing(0, competition_long_marker());
                        outgoing(1, competition_long_marker());
                        c.set(14, (competition_id * 0x6c) + 0x1a);
                        outgoing(2, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        c.set(13, (field16 * 0x128) + 0xe1);
                        outgoing(3, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                        outgoing(4, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                        c.set(16, persons());
                        c.set(16, u8((c.r(16) + (field17 * 0x128)) + 0x116));
                        c.set(16, sb(c.r(16)));
                        outgoing(5, c.r(16));
                        c.set(16, clubs());
                        outgoing(6, c.r(16) + ((field19 * 0x248) + 0x221));
                        outgoing(7, c.r(3));
                        outgoing(8, subject_full);
                        outgoing(9, home_short());
                        outgoing(10, competitions() + c.r(14));
                        outgoing(11, persons() + c.r(13));
                        outgoing(12, persons() + ((field17 * 0x128) + 0xe1));
                        say(body, phrase(0x2f25f), 0x1068e0a4, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x7f))), sb(u8(at(0x7f))), home_marker(), home_marker()});
                        return;
                    }
                }
            }
        }
    }
    say(body, phrase(0x3024b), 0x1068f290, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
}
}
