// The season outlook stories.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

void Story::story_63() {
    // Story code 0x63: speculation about a staff member.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(own_club), 0) != 0) {
            say(body, phrase(0x36586), 0x1069520c, {home_marker(), home_marker(), home_short(), clubs()});
            return;
        }
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x3661c), 0x10695264, {home_marker(), home_marker(), home_short(), other_full, clubs()});
            return;
        }
        call(0x10168a00, 0x10695288, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1b}});
        outgoing(0, other_full);
        say(body, phrase(0x366e7), 0x106952e4, {0x64, 0x64, home_marker(), home_marker(), nationality, home_short()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            if (cmp(field15, 0) == 0) {
                say(body, phrase(0x36355), 0x10695048, {home_marker(), home_marker(), home_short(), clubs()});
                return;
            }
            say(body, phrase(0x36292), 0x10694ffc, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x364cf), 0x106951b4, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
            return;
        }
        outgoing(0, nation() + 0xe1);
        outgoing(1, home_short());
        outgoing(2, competition_long_name());
        say(body, phrase(0x363f0), 0x10695124, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
        return;
    }
    if (cmp(sb(c.r(16)), 0) == 0) {
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x369d1), 0x106954b4, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
            return;
        }
        say(body, phrase(0x36912), 0x10695424, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
        return;
    }
    if (cmp(field15, 0) == 0) {
        say(body, phrase(0x36877), 0x1069538c, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    say(body, phrase(0x367d4), 0x10695340, {home_marker(), home_marker(), home_short(), clubs()});
}

void Story::story_61() {
    // Story code 0x61: speculation about a staff member (media dubious).
    if (cmp(kind, 1) != 0) {
        if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
            if (cmp(sb(c.r(16)), 0) == 0) {
                say(body, phrase(0x36b23), 0x106955b8, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
                return;
            }
            say(body, phrase(0x36a88), 0x10695528, {home_marker(), home_marker(), home_short(), clubs()});
            return;
        }
    } else {
        if (cmp(sb(own_club), 0) != 0) {
            say(body, phrase(0x36bda), 0x10695610, {home_marker(), home_marker(), home_short(), clubs()});
            return;
        }
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x36c7c), 0x10695668, {home_marker(), home_marker(), home_short(), other_full, clubs()});
            return;
        }
        call(0x10168a00, 0x1069568c, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1b}});
        outgoing(0, other_full);
        say(body, phrase(0x36d4b), 0x106956e8, {0x64, 0x64, home_marker(), home_marker(), nationality, home_short()});
        return;
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        say(body, phrase(0x36e3c), 0x1069573c, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    call(0x10168a00, 0x10695760, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    say(body, phrase(0x36ee1), 0x106957b4, {0x64, 0x64, home_marker(), home_marker(), nationality, home_short()});
}

void Story::story_5d() {
    // Story code 0x5d: a staff member considered by many in the media.
    if (cmp(kind, 0) != 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            if (cmp(field15, 0) == 0) {
                say(body, phrase(0x3762a), 0x10695e98, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                return;
            }
            say(body, phrase(0x3758f), 0x10695e14, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        call(0x10168a00, 0x10695ebc, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        if (cmp(field15, 0) == 0) {
            outgoing(0, home_short());
            outgoing(1, nationality);
            outgoing(2, competition_long_name());
            say(body, phrase(0x377c5), 0x10695ff8, {home_marker(), home_marker(), 0x64, 0x64, competition_long_marker(), competition_long_marker()});
            return;
        }
        outgoing(0, home_short());
        outgoing(1, nationality);
        outgoing(2, competition_long_name());
        say(body, phrase(0x376e5), 0x10695f5c, {home_marker(), home_marker(), 0x64, 0x64, competition_long_marker(), competition_long_marker()});
        return;
    }
    if (cmp(staff_id, -1) > 0 && cmp(field19, -1) > 0 && cmp(field1a, -1) > 0) {
        call(0x100c6bf0, 0x106957f8, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x73)}});
        c.set(0, c.r(3));
        if (cmp(field15, 0) == 0) {
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(0));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x3708b), 0x106959d8, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x73))), sb(u8(at(0x73))), home_marker(), home_marker()});
            return;
        }
        outgoing(0, competition_marker());
        outgoing(1, competition_marker());
        outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
        outgoing(3, c.r(3));
        outgoing(4, subject_full);
        outgoing(5, home_short());
        outgoing(6, competition_name());
        say(body, phrase(0x36fa8), 0x106958ec, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x73))), sb(u8(at(0x73))), home_marker(), home_marker()});
        return;
    }
    if (cmp(staff_id, -1) > 0 && cmp(sb(c.r(13)), 0) != 0) {
        if (cmp(field15, 0) == 0) {
            outgoing(0, competition_name());
            say(body, phrase(0x3723a), 0x10695b08, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
            return;
        }
        outgoing(0, competition_name());
        say(body, phrase(0x37176), 0x10695a7c, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
        return;
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x37398), 0x10695c20, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        say(body, phrase(0x372ff), 0x10695b9c, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    call(0x10168a00, 0x10695c44, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1b}});
    if (cmp(field15, 0) == 0) {
        outgoing(0, nationality);
        outgoing(1, home_short());
        outgoing(2, competition_long_name());
        say(body, phrase(0x374e0), 0x10695d80, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
        return;
    }
    outgoing(0, nationality);
    outgoing(1, home_short());
    outgoing(2, competition_long_name());
    say(body, phrase(0x3742e), 0x10695ce4, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
}

void Story::story_62() {
    // Story code 0x62: the media convinced about a staff member.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            if (cmp(field15, 0) == 0) {
                say(body, phrase(0x37f62), 0x106965ac, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                return;
            }
            say(body, phrase(0x37e81), 0x10696528, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        call(0x10168a00, 0x106965d0, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        if (cmp(field15, 0) == 0) {
            outgoing(0, home_short());
            outgoing(1, nationality);
            outgoing(2, competition_long_name());
            say(body, phrase(0x38142), 0x1069670c, {home_marker(), home_marker(), 0x64, 0x64, competition_long_marker(), competition_long_marker()});
            return;
        }
        outgoing(0, home_short());
        outgoing(1, nationality);
        outgoing(2, competition_long_name());
        say(body, phrase(0x3803f), 0x10696670, {home_marker(), home_marker(), 0x64, 0x64, competition_long_marker(), competition_long_marker()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                if (cmp(sb(c.r(16)), 0) == 0) {
                    if (cmp(field15, 0) == 0) {
                        say(body, phrase(0x37db0), 0x10696494, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
                        return;
                    }
                    say(body, phrase(0x37cdc), 0x10696404, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
                    return;
                }
                if (cmp(field15, 0) == 0) {
                    say(body, phrase(0x37c27), 0x1069636c, {home_marker(), home_marker(), home_short(), clubs()});
                    return;
                }
                say(body, phrase(0x37b6f), 0x10696320, {home_marker(), home_marker(), home_short(), clubs()});
                return;
            } else {
                outgoing(0, competition_name());
                say(body, phrase(0x37a96), 0x106962c4, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                return;
            }
        } else {
            call(0x100c6bf0, 0x10696048, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x72)}});
            c.set(0, c.r(3));
            if (cmp(field15, 0) == 0) {
                outgoing(0, competition_marker());
                outgoing(1, competition_marker());
                outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                outgoing(3, c.r(0));
                outgoing(4, subject_full);
                outgoing(5, home_short());
                outgoing(6, competition_name());
                say(body, phrase(0x37997), 0x10696228, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x72))), sb(u8(at(0x72))), home_marker(), home_marker()});
                return;
            }
            outgoing(0, competition_marker());
            outgoing(1, competition_marker());
            outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(3, c.r(3));
            outgoing(4, subject_full);
            outgoing(5, home_short());
            outgoing(6, competition_name());
            say(body, phrase(0x378a2), 0x1069613c, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x72))), sb(u8(at(0x72))), home_marker(), home_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x38308), 0x10696824, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        say(body, phrase(0x38241), 0x106967a0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    call(0x10168a00, 0x10696848, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    if (cmp(field15, 0) == 0) {
        outgoing(0, nationality);
        outgoing(1, home_short());
        outgoing(2, competition_long_name());
        say(body, phrase(0x384b5), 0x10696984, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
        return;
    }
    outgoing(0, nationality);
    outgoing(1, home_short());
    outgoing(2, competition_long_name());
    say(body, phrase(0x383cc), 0x106968e8, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
}

void Story::story_40() {
    // Story code 0x40: media claims about a staff member.
    if (cmp(kind, 0) != 0) {
        say(body, phrase(0x387ec), 0x10696ba0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    if (cmp(staff_id, -1) > 0 && cmp(field19, -1) > 0 && cmp(field1a, -1) > 0) {
        call(0x100c6bf0, 0x106969c8, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x71)}});
        c.set(0, c.r(3));
        outgoing(0, clubs() + ((field19 * 0x248) + 0x221));
        outgoing(1, c.r(3));
        outgoing(2, subject_full);
        outgoing(3, home_short());
        say(body, phrase(0x3859b), 0x10696a70, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x71))), sb(u8(at(0x71))), home_marker(), home_marker()});
        return;
    }
    if (cmp(staff_id, -1) > 0 && cmp(sb(c.r(13)), 0) != 0) {
        say(body, phrase(0x38685), 0x10696ad0, {home_marker(), home_marker(), subject_full, home_short(), clubs()});
        return;
    }
    say(body, phrase(0x38742), 0x10696b1c, {home_marker(), home_marker(), home_short(), clubs()});
}

void Story::story_41() {
    // Story code 0x41: media claims about a staff member (nationality).
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x38c31), 0x10696e6c, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        outgoing(0, nation() + 0xe1);
        outgoing(1, home_short());
        outgoing(2, competition_long_name());
        say(body, phrase(0x38d1e), 0x10696f40, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                if (cmp(sb(c.r(16)), 0) == 0) {
                    say(body, phrase(0x38b6c), 0x10696de0, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
                    return;
                }
                say(body, phrase(0x38ac3), 0x10696d50, {home_marker(), home_marker(), home_short(), clubs()});
                return;
            } else {
                say(body, phrase(0x389b3), 0x10696cfc, {home_marker(), home_marker(), subject_full, home_short(), subject_short});
                return;
            }
        } else {
            call(0x100c6bf0, 0x10696bf0, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x70)}});
            c.set(0, c.r(3));
            outgoing(0, clubs() + ((field19 * 0x248) + 0x221));
            outgoing(1, c.r(3));
            outgoing(2, subject_full);
            outgoing(3, home_short());
            say(body, phrase(0x388ae), 0x10696c98, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x70))), sb(u8(at(0x70))), home_marker(), home_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x38ee6), 0x10697058, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        say(body, phrase(0x38e27), 0x10696fd4, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
        return;
    }
    call(0x10168a00, 0x1069707c, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    if (cmp(field15, 0) == 0) {
        outgoing(0, nationality);
        outgoing(1, home_short());
        outgoing(2, competition_long_name());
        say(body, phrase(0x39080), 0x106971b8, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
        return;
    }
    outgoing(0, nationality);
    outgoing(1, home_short());
    outgoing(2, competition_long_name());
    say(body, phrase(0x38f9f), 0x1069711c, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
}
}
