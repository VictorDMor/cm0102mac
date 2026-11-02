// The long fixture-based story.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

void Story::story_3d() {
    // Story code 0x3d: the longest: a fixture and what the supporters think of it.
    if (cmp(sb(has_match), 0) == 0) {
        put8(toc(-0x7bb8), 0);
        return;
    }
    score(0x106936f0);
    if (cmp(i16(at(0xd8)), 0) != 0 || cmp(i16(at(0xda)), 0) != 0) {
        call(0x104769d0, 0x10693730, {{3, competitions() + (u32(at(0xd4)) * 0x6c)}, {4, i16(at(0xd8))}, {5, i16(at(0xda))}, {6, 1}, {7, stage_name}, {8, 0xc8}});
        call(0x1077dc90, 0x10693740, {{3, stage_name}, {4, phrase(0x247bc)}});
        if (cmp(c.r(3), 0) == 0) {
            put8(at(0x304), 0x66);
            call(0x10476ec0, 0x10693788, {{3, 0}, {4, i16(at(0xd8))}, {5, i16(at(0xda))}, {6, 1}});
            c.set(0, c.r(3));
            if (cmp(kind, 2) == 0) {
                if (cmp(home_goals(), away_goals()) != 0) {
                    outgoing(0, sb(outlet_flag()));
                    outgoing(1, sb(outlet_flag()));
                    c.set(13, clubs());
                    outgoing(2, c.r(13) + ((club_id * 0x248) + 0x221));
                    outgoing(3, score_text);
                    outgoing(4, stage_name);
                    outgoing(5, rival_long());
                    outgoing(6, outlet_text);
                    say(body, phrase(0x35921), 0x106940e4, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    return;
                }
                outgoing(0, sb(outlet_flag()));
                outgoing(1, sb(outlet_flag()));
                c.set(13, clubs());
                outgoing(2, c.r(13) + ((club_id * 0x248) + 0x221));
                outgoing(3, score_text);
                outgoing(4, stage_name);
                outgoing(5, rival_long());
                outgoing(6, outlet_text);
                say(body, phrase(0x35827), 0x10693ea8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                level_ending(0x10693ef4);
                return;
            }
            if (cmp(kind, 2) >= 0) {
                if (cmp(home_goals(), away_goals()) != 0) {
                    outgoing(0, home_short());
                    outgoing(1, score_text);
                    outgoing(2, stage_name);
                    outgoing(3, rival_long());
                    say(body, phrase(0x35b07), 0x106943c8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    return;
                }
                outgoing(0, home_short());
                outgoing(1, score_text);
                outgoing(2, stage_name);
                outgoing(3, rival_long());
                say(body, phrase(0x35a28), 0x106941ac, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                level_ending(0x106941f8);
                return;
            } else {
                if (cmp(kind, 0) == 0) {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, home_marker());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        outgoing(4, home_short());
                        say(body, phrase(0x3551e), 0x10693a9c, {0x64, sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker(), home_marker()});
                        return;
                    }
                    outgoing(0, home_marker());
                    outgoing(1, score_text);
                    outgoing(2, stage_name);
                    outgoing(3, rival_long());
                    outgoing(4, home_short());
                    say(body, phrase(0x3543a), 0x10693878, {0x64, sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker(), home_marker()});
                    level_ending(0x106938c4);
                    return;
                }
                if (cmp(kind, 0) < 0) {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, home_short());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        say(body, phrase(0x35b07), 0x106943c8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        return;
                    }
                    outgoing(0, home_short());
                    outgoing(1, score_text);
                    outgoing(2, stage_name);
                    outgoing(3, rival_long());
                    say(body, phrase(0x35a28), 0x106941ac, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    level_ending(0x106941f8);
                    return;
                } else {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, rival_long_marker());
                        c.set(13, rival_id());
                        c.set(14, clubs());
                        c.set(13, c.r(14) + (c.r(13) * 0x248));
                        outgoing(1, sb(u8(c.r(13) + 0x242)));
                        outgoing(2, outlet_text);
                        outgoing(3, home_short());
                        outgoing(4, score_text);
                        outgoing(5, stage_name);
                        outgoing(6, rival_long());
                        say(body, phrase(0x35713), 0x10693dc0, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0))});
                        return;
                    }
                    outgoing(0, rival_long_marker());
                    c.set(13, rival_id());
                    c.set(14, clubs());
                    c.set(13, c.r(14) + (c.r(13) * 0x248));
                    outgoing(1, sb(u8(c.r(13) + 0x242)));
                    outgoing(2, outlet_text);
                    outgoing(3, home_short());
                    outgoing(4, score_text);
                    outgoing(5, stage_name);
                    outgoing(6, rival_long());
                    say(body, phrase(0x3560c), 0x10693b84, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0))});
                    level_ending(0x10693bd0);
                    return;
                }
            }
        } else {
            call(0x1077dc90, 0x10693764, {{3, stage_name}, {4, phrase(0x247c5)}});
            if (cmp(c.r(3), 0) != 0) {
                call(0x10476ec0, 0x10693788, {{3, 0}, {4, i16(at(0xd8))}, {5, i16(at(0xda))}, {6, 1}});
                c.set(0, c.r(3));
                if (cmp(kind, 2) == 0) {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, sb(outlet_flag()));
                        outgoing(1, sb(outlet_flag()));
                        c.set(13, clubs());
                        outgoing(2, c.r(13) + ((club_id * 0x248) + 0x221));
                        outgoing(3, score_text);
                        outgoing(4, stage_name);
                        outgoing(5, rival_long());
                        outgoing(6, outlet_text);
                        say(body, phrase(0x35921), 0x106940e4, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        return;
                    }
                    outgoing(0, sb(outlet_flag()));
                    outgoing(1, sb(outlet_flag()));
                    c.set(13, clubs());
                    outgoing(2, c.r(13) + ((club_id * 0x248) + 0x221));
                    outgoing(3, score_text);
                    outgoing(4, stage_name);
                    outgoing(5, rival_long());
                    outgoing(6, outlet_text);
                    say(body, phrase(0x35827), 0x10693ea8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    level_ending(0x10693ef4);
                    return;
                }
                if (cmp(kind, 2) >= 0) {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, home_short());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        say(body, phrase(0x35b07), 0x106943c8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        return;
                    }
                    outgoing(0, home_short());
                    outgoing(1, score_text);
                    outgoing(2, stage_name);
                    outgoing(3, rival_long());
                    say(body, phrase(0x35a28), 0x106941ac, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    level_ending(0x106941f8);
                    return;
                } else {
                    if (cmp(kind, 0) == 0) {
                        if (cmp(home_goals(), away_goals()) != 0) {
                            outgoing(0, home_marker());
                            outgoing(1, score_text);
                            outgoing(2, stage_name);
                            outgoing(3, rival_long());
                            outgoing(4, home_short());
                            say(body, phrase(0x3551e), 0x10693a9c, {0x64, sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker(), home_marker()});
                            return;
                        }
                        outgoing(0, home_marker());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        outgoing(4, home_short());
                        say(body, phrase(0x3543a), 0x10693878, {0x64, sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker(), home_marker()});
                        level_ending(0x106938c4);
                        return;
                    }
                    if (cmp(kind, 0) < 0) {
                        if (cmp(home_goals(), away_goals()) != 0) {
                            outgoing(0, home_short());
                            outgoing(1, score_text);
                            outgoing(2, stage_name);
                            outgoing(3, rival_long());
                            say(body, phrase(0x35b07), 0x106943c8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                            return;
                        }
                        outgoing(0, home_short());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        say(body, phrase(0x35a28), 0x106941ac, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        level_ending(0x106941f8);
                        return;
                    } else {
                        if (cmp(home_goals(), away_goals()) != 0) {
                            outgoing(0, rival_long_marker());
                            c.set(13, rival_id());
                            c.set(14, clubs());
                            c.set(13, c.r(14) + (c.r(13) * 0x248));
                            outgoing(1, sb(u8(c.r(13) + 0x242)));
                            outgoing(2, outlet_text);
                            outgoing(3, home_short());
                            outgoing(4, score_text);
                            outgoing(5, stage_name);
                            outgoing(6, rival_long());
                            say(body, phrase(0x35713), 0x10693dc0, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0))});
                            return;
                        }
                        outgoing(0, rival_long_marker());
                        c.set(13, rival_id());
                        c.set(14, clubs());
                        c.set(13, c.r(14) + (c.r(13) * 0x248));
                        outgoing(1, sb(u8(c.r(13) + 0x242)));
                        outgoing(2, outlet_text);
                        outgoing(3, home_short());
                        outgoing(4, score_text);
                        outgoing(5, stage_name);
                        outgoing(6, rival_long());
                        say(body, phrase(0x3560c), 0x10693b84, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0))});
                        level_ending(0x10693bd0);
                        return;
                    }
                }
            } else {
                put8(at(0x304), 0x72);
                call(0x10476ec0, 0x10693788, {{3, 0}, {4, i16(at(0xd8))}, {5, i16(at(0xda))}, {6, 1}});
                c.set(0, c.r(3));
                if (cmp(kind, 2) == 0) {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, sb(outlet_flag()));
                        outgoing(1, sb(outlet_flag()));
                        c.set(13, clubs());
                        outgoing(2, c.r(13) + ((club_id * 0x248) + 0x221));
                        outgoing(3, score_text);
                        outgoing(4, stage_name);
                        outgoing(5, rival_long());
                        outgoing(6, outlet_text);
                        say(body, phrase(0x35921), 0x106940e4, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        return;
                    }
                    outgoing(0, sb(outlet_flag()));
                    outgoing(1, sb(outlet_flag()));
                    c.set(13, clubs());
                    outgoing(2, c.r(13) + ((club_id * 0x248) + 0x221));
                    outgoing(3, score_text);
                    outgoing(4, stage_name);
                    outgoing(5, rival_long());
                    outgoing(6, outlet_text);
                    say(body, phrase(0x35827), 0x10693ea8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    level_ending(0x10693ef4);
                    return;
                }
                if (cmp(kind, 2) >= 0) {
                    if (cmp(home_goals(), away_goals()) != 0) {
                        outgoing(0, home_short());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        say(body, phrase(0x35b07), 0x106943c8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        return;
                    }
                    outgoing(0, home_short());
                    outgoing(1, score_text);
                    outgoing(2, stage_name);
                    outgoing(3, rival_long());
                    say(body, phrase(0x35a28), 0x106941ac, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                    level_ending(0x106941f8);
                    return;
                } else {
                    if (cmp(kind, 0) == 0) {
                        if (cmp(home_goals(), away_goals()) != 0) {
                            outgoing(0, home_marker());
                            outgoing(1, score_text);
                            outgoing(2, stage_name);
                            outgoing(3, rival_long());
                            outgoing(4, home_short());
                            say(body, phrase(0x3551e), 0x10693a9c, {0x64, sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker(), home_marker()});
                            return;
                        }
                        outgoing(0, home_marker());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        outgoing(4, home_short());
                        say(body, phrase(0x3543a), 0x10693878, {0x64, sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker(), home_marker()});
                        level_ending(0x106938c4);
                        return;
                    }
                    if (cmp(kind, 0) < 0) {
                        if (cmp(home_goals(), away_goals()) != 0) {
                            outgoing(0, home_short());
                            outgoing(1, score_text);
                            outgoing(2, stage_name);
                            outgoing(3, rival_long());
                            say(body, phrase(0x35b07), 0x106943c8, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                            return;
                        }
                        outgoing(0, home_short());
                        outgoing(1, score_text);
                        outgoing(2, stage_name);
                        outgoing(3, rival_long());
                        say(body, phrase(0x35a28), 0x106941ac, {home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0)), rival_long_marker(), rival_long_marker()});
                        level_ending(0x106941f8);
                        return;
                    } else {
                        if (cmp(home_goals(), away_goals()) != 0) {
                            outgoing(0, rival_long_marker());
                            c.set(13, rival_id());
                            c.set(14, clubs());
                            c.set(13, c.r(14) + (c.r(13) * 0x248));
                            outgoing(1, sb(u8(c.r(13) + 0x242)));
                            outgoing(2, outlet_text);
                            outgoing(3, home_short());
                            outgoing(4, score_text);
                            outgoing(5, stage_name);
                            outgoing(6, rival_long());
                            say(body, phrase(0x35713), 0x10693dc0, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0))});
                            return;
                        }
                        outgoing(0, rival_long_marker());
                        c.set(13, rival_id());
                        c.set(14, clubs());
                        c.set(13, c.r(14) + (c.r(13) * 0x248));
                        outgoing(1, sb(u8(c.r(13) + 0x242)));
                        outgoing(2, outlet_text);
                        outgoing(3, home_short());
                        outgoing(4, score_text);
                        outgoing(5, stage_name);
                        outgoing(6, rival_long());
                        say(body, phrase(0x3560c), 0x10693b84, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), sb(c.r(0)), sb(c.r(0))});
                        level_ending(0x10693bd0);
                        return;
                    }
                }
            }
        }
    } else {
        if (cmp(kind, 2) == 0) {
            if (cmp(home_goals(), away_goals()) != 0) {
                outgoing(0, home_short());
                outgoing(1, score_text);
                outgoing(2, rival_long());
                outgoing(3, outlet_text);
                say(body, phrase(0x36032), 0x10694c94, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
                return;
            }
            outgoing(0, home_short());
            outgoing(1, score_text);
            outgoing(2, rival_long());
            outgoing(3, outlet_text);
            say(body, phrase(0x35f59), 0x10694a70, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            level_ending(0x10694abc);
            return;
        }
        if (cmp(kind, 2) < 0) {
            if (cmp(kind, 0) == 0) {
                if (cmp(home_goals(), away_goals()) != 0) {
                    outgoing(0, rival_long());
                    outgoing(1, home_short());
                    say(body, phrase(0x35cb3), 0x106946ac, {0x64, rival_long_marker(), rival_long_marker(), home_marker(), home_marker(), score_text});
                    return;
                }
                outgoing(0, rival_long());
                outgoing(1, home_short());
                say(body, phrase(0x35bf0), 0x106944a0, {0x64, rival_long_marker(), rival_long_marker(), home_marker(), home_marker(), score_text});
                level_ending(0x106944ec);
                return;
            }
            if (cmp(kind, 0) < 0) {
            } else {
                if (cmp(home_goals(), away_goals()) != 0) {
                    outgoing(0, outlet_text);
                    outgoing(1, home_short());
                    outgoing(2, score_text);
                    outgoing(3, rival_long());
                    say(body, phrase(0x35e66), 0x106949a0, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), rival_long_marker(), rival_long_marker()});
                    return;
                }
                outgoing(0, outlet_text);
                outgoing(1, home_short());
                outgoing(2, score_text);
                outgoing(3, rival_long());
                say(body, phrase(0x35d80), 0x1069477c, {sb(outlet_flag()), sb(outlet_flag()), home_marker(), home_marker(), rival_long_marker(), rival_long_marker()});
                level_ending(0x106947c8);
                return;
            }
        }
    }
    if (cmp(home_goals(), away_goals()) != 0) {
        outgoing(0, rival_long());
        say(body, phrase(0x361d0), 0x10694f48, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), score_text});
        return;
    }
    outgoing(0, rival_long());
    say(body, phrase(0x3610e), 0x10694d44, {home_marker(), home_marker(), rival_long_marker(), rival_long_marker(), home_short(), score_text});
    level_ending(0x10694d90);
    return;
}
}
