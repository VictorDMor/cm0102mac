// The money, transfer and contract stories.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

void Story::story_5a() {
    // Story code 0x5a: a result in a competition and what the staff say about it.
    if (cmp(kind, 1) == 0) {
        if (cmp(field15, 0) == 0) {
            if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                say(body, phrase(0x31838), 0x10690b70, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                return;
            } else {
                say(body, phrase(0x318c6), 0x10690bf4, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                return;
            }
        } else {
            if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                outgoing(0, home_short());
                outgoing(1, competition_name());
                outgoing(2, home_short());
                say(body, phrase(0x314fd), 0x10690850, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_marker(), home_marker()});
                return;
            } else {
                if (cmp(field17, -1) <= 0) {
                    outgoing(0, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                    outgoing(1, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                    outgoing(2, home_short());
                    outgoing(3, competition_name());
                    outgoing(4, home_short());
                    outgoing(5, persons() + ((field16 * 0x128) + 0xe1));
                    c.set(13, home());
                    say(body, phrase(0x31727), 0x10690ab4, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_marker(), competition_marker(), home_marker(), home_marker()});
                    return;
                }
                outgoing(0, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                outgoing(1, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                outgoing(2, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                outgoing(3, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                outgoing(4, home_short());
                outgoing(5, competition_name());
                c.set(13, clubs());
                outgoing(6, c.r(13) + ((club_id * 0x248) + 0x221));
                outgoing(7, persons() + ((field16 * 0x128) + 0xe1));
                outgoing(8, persons() + ((field17 * 0x128) + 0xe1));
                c.set(13, home());
                say(body, phrase(0x315ed), 0x106909a8, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_marker(), competition_marker(), home_marker(), home_marker()});
                return;
            }
        }
    } else {
        if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
            if (cmp(field15, 0) == 0) {
                if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                    if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                            say(body, phrase(0x30e77), 0x10690064, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                            return;
                        } else {
                            outgoing(0, competition_long_name());
                            say(body, phrase(0x30db3), 0x1068ffe0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), subject_full, home_short()});
                            return;
                        }
                    } else {
                        call(0x100c6bf0, 0x1068fe58, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x77)}});
                        c.set(0, c.r(3));
                        outgoing(0, competition_long_marker());
                        outgoing(1, competition_long_marker());
                        outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                        outgoing(3, c.r(3));
                        outgoing(4, subject_full);
                        outgoing(5, home_short());
                        outgoing(6, competition_long_name());
                        say(body, phrase(0x30cd5), 0x1068ff44, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x77))), sb(u8(at(0x77))), home_marker(), home_marker()});
                        return;
                    }
                } else {
                    if (cmp(field17, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                                outgoing(0, home_short());
                                outgoing(1, competition_name());
                                outgoing(2, persons() + ((field16 * 0x128) + 0xe1));
                                say(body, phrase(0x3143a), 0x10690748, {home_marker(), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            } else {
                                outgoing(0, subject_full);
                                outgoing(1, home_short());
                                outgoing(2, competition_name());
                                outgoing(3, persons() + ((field16 * 0x128) + 0xe1));
                                say(body, phrase(0x3134e), 0x10690680, {home_marker(), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            }
                        } else {
                            call(0x100c6bf0, 0x10690470, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x75)}});
                            c.set(0, c.r(3));
                            outgoing(0, competition_marker());
                            c.set(13, (field19 * 0x248) + 0x221);
                            outgoing(1, competition_marker());
                            outgoing(2, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                            outgoing(3, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                            c.set(14, clubs());
                            c.set(13, c.r(14) + c.r(13));
                            outgoing(4, c.r(13));
                            outgoing(5, c.r(3));
                            outgoing(6, subject_full);
                            outgoing(7, home_short());
                            outgoing(8, competition_name());
                            outgoing(9, persons() + ((field16 * 0x128) + 0xe1));
                            say(body, phrase(0x31248), 0x106905a0, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x75))), sb(u8(at(0x75))), home_marker(), home_marker()});
                            return;
                        }
                    } else {
                        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                                outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(2, home_short());
                                outgoing(3, competition_name());
                                outgoing(4, persons() + ((field16 * 0x128) + 0xe1));
                                outgoing(5, persons() + ((field17 * 0x128) + 0xe1));
                                c.set(13, home());
                                say(body, phrase(0x3115c), 0x10690440, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            } else {
                                outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(2, subject_full);
                                outgoing(3, home_short());
                                outgoing(4, competition_name());
                                outgoing(5, persons() + ((field16 * 0x128) + 0xe1));
                                outgoing(6, persons() + ((field17 * 0x128) + 0xe1));
                                c.set(13, home());
                                say(body, phrase(0x31047), 0x10690334, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            }
                        } else {
                            call(0x100c6bf0, 0x1069009c, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x76)}});
                            c.set(0, c.r(3));
                            outgoing(0, competition_marker());
                            outgoing(1, competition_marker());
                            c.set(14, (competition_id * 0x6c) + 0x4d);
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
                            say(body, phrase(0x30f18), 0x10690210, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x76))), sb(u8(at(0x76))), home_marker(), home_marker()});
                            return;
                        }
                    }
                }
            } else {
                if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
                    if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                            say(body, phrase(0x30641), 0x1068f70c, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                            return;
                        } else {
                            outgoing(0, competition_long_name());
                            say(body, phrase(0x3057b), 0x1068f688, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), subject_full, home_short()});
                            return;
                        }
                    } else {
                        call(0x100c6bf0, 0x1068f500, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x7a)}});
                        c.set(0, c.r(3));
                        outgoing(0, competition_long_marker());
                        outgoing(1, competition_long_marker());
                        outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                        outgoing(3, c.r(3));
                        outgoing(4, subject_full);
                        outgoing(5, home_short());
                        outgoing(6, competition_long_name());
                        say(body, phrase(0x3049b), 0x1068f5ec, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x7a))), sb(u8(at(0x7a))), home_marker(), home_marker()});
                        return;
                    }
                } else {
                    if (cmp(field17, -1) <= 0) {
                        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                                outgoing(0, home_short());
                                outgoing(1, competition_name());
                                outgoing(2, persons() + ((field16 * 0x128) + 0xe1));
                                say(body, phrase(0x30c10), 0x1068fdf0, {home_marker(), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            } else {
                                outgoing(0, subject_full);
                                outgoing(1, home_short());
                                outgoing(2, competition_name());
                                outgoing(3, persons() + ((field16 * 0x128) + 0xe1));
                                say(body, phrase(0x30b22), 0x1068fd28, {home_marker(), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            }
                        } else {
                            call(0x100c6bf0, 0x1068fb18, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x78)}});
                            c.set(0, c.r(3));
                            outgoing(0, competition_marker());
                            c.set(13, (field19 * 0x248) + 0x221);
                            outgoing(1, competition_marker());
                            outgoing(2, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                            outgoing(3, sb(u8((persons() + (field16 * 0x128)) + 0x116)));
                            c.set(14, clubs());
                            c.set(13, c.r(14) + c.r(13));
                            outgoing(4, c.r(13));
                            outgoing(5, c.r(3));
                            outgoing(6, subject_full);
                            outgoing(7, home_short());
                            outgoing(8, competition_name());
                            outgoing(9, persons() + ((field16 * 0x128) + 0xe1));
                            say(body, phrase(0x30a1a), 0x1068fc48, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x78))), sb(u8(at(0x78))), home_marker(), home_marker()});
                            return;
                        }
                    } else {
                        if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                            if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                                outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(2, home_short());
                                outgoing(3, competition_name());
                                outgoing(4, persons() + ((field16 * 0x128) + 0xe1));
                                outgoing(5, persons() + ((field17 * 0x128) + 0xe1));
                                c.set(13, home());
                                say(body, phrase(0x3092c), 0x1068fae8, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            } else {
                                outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
                                outgoing(2, subject_full);
                                outgoing(3, home_short());
                                outgoing(4, competition_name());
                                outgoing(5, persons() + ((field16 * 0x128) + 0xe1));
                                outgoing(6, persons() + ((field17 * 0x128) + 0xe1));
                                c.set(13, home());
                                say(body, phrase(0x30815), 0x1068f9dc, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_marker(), competition_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                                return;
                            }
                        } else {
                            call(0x100c6bf0, 0x1068f744, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x79)}});
                            c.set(0, c.r(3));
                            outgoing(0, competition_marker());
                            outgoing(1, competition_marker());
                            c.set(14, (competition_id * 0x6c) + 0x4d);
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
                            say(body, phrase(0x306e4), 0x1068f8b8, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x79))), sb(u8(at(0x79))), home_marker(), home_marker()});
                            return;
                        }
                    }
                }
            }
        }
    }
    if (cmp(field15, 0) != 0) {
        if (cmp(field16, u32(nation() + 0x68)) == 0 || cmp(field17, u32(nation() + 0x68)) == 0) {
            outgoing(0, home_marker());
            outgoing(1, home_marker());
            outgoing(2, home_short());
            outgoing(3, competition_long_name());
            outgoing(4, outlet_text);
            outgoing(5, home_short());
            say(body, phrase(0x31947), 0x10690d1c, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            return;
        } else {
            outgoing(0, home_marker());
            outgoing(1, home_marker());
            outgoing(2, home_short());
            outgoing(3, competition_long_name());
            outgoing(4, outlet_text);
            outgoing(5, home_short());
            say(body, phrase(0x31a9a), 0x10690e04, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            return;
        }
    } else {
        if (cmp(field16, u32(nation() + 0x68)) != 0 && cmp(field17, u32(nation() + 0x68)) != 0) {
            if (cmp(field17, -1) <= 0) {
                outgoing(0, sb(outlet_flag()));
                outgoing(1, sb(outlet_flag()));
                outgoing(2, home_marker());
                outgoing(3, home_marker());
                outgoing(4, home_short());
                c.set(13, competitions());
                outgoing(5, c.r(13) + ((competition_id * 0x6c) + 0x1a));
                c.set(13, persons());
                outgoing(6, c.r(13) + ((field16 * 0x128) + 0xe1));
                outgoing(7, outlet_text);
                outgoing(8, home_short());
                c.set(13, home());
                say(body, phrase(0x31ebc), 0x10691184, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
                return;
            }
            c.set(13, outlet_text);
            outgoing(0, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
            outgoing(1, sb(u8((persons() + (field17 * 0x128)) + 0x116)));
            c.set(16, (field16 * 0x128) + 0xe1);
            outgoing(2, sb(outlet_flag()));
            c.set(14, (field17 * 0x128) + 0xe1);
            outgoing(3, sb(outlet_flag()));
            outgoing(4, home_marker());
            outgoing(5, home_marker());
            c.set(17, clubs());
            outgoing(6, c.r(17) + ((club_id * 0x248) + 0x221));
            outgoing(7, competition_long_name());
            outgoing(8, persons() + c.r(16));
            outgoing(9, persons() + c.r(14));
            outgoing(10, c.r(13));
            outgoing(11, home_short());
            c.set(13, home());
            say(body, phrase(0x31d26), 0x10691058, {sb(u8(c.r(13) + 0x23b)), home_marker(), competition_long_marker(), competition_long_marker(), sb(u8((persons() + (field16 * 0x128)) + 0x116)), sb(u8((persons() + (field16 * 0x128)) + 0x116))});
            return;
        }
    }
    outgoing(0, home_short());
    outgoing(1, competition_long_name());
    outgoing(2, outlet_text);
    say(body, phrase(0x31bf1), 0x10690ee0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
}

void Story::story_5c() {
    // Story code 0x5c: the defeat of an opposition team, with a second paragraph.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            if (cmp(field15, 0) == 0) {
                say(body, phrase(0x326ab), 0x106918c8, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            } else {
                say(body, phrase(0x325e1), 0x10691844, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            }
        } else {
            call(0x10168a00, 0x106918ec, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
            if (cmp(field15, 0) == 0) {
                outgoing(0, home_short());
                outgoing(1, competition_name());
                outgoing(2, nationality);
                say(body, phrase(0x3281a), 0x10691a28, {home_marker(), home_marker(), competition_marker(), competition_marker(), 0x64, 0x64});
            } else {
                outgoing(0, home_short());
                outgoing(1, competition_name());
                outgoing(2, nationality);
                say(body, phrase(0x3272e), 0x1069198c, {home_marker(), home_marker(), competition_marker(), competition_marker(), 0x64, 0x64});
            }
        }
    } else {
        if (cmp(kind, 1) >= 0 || cmp(kind, 0) < 0) {
            if (cmp(field15, 0) == 0) {
                outgoing(0, home_marker());
                outgoing(1, home_marker());
                outgoing(2, nation_marker());
                c.set(13, nation());
                outgoing(3, sb(u8(c.r(13) + 0x116)));
                c.set(13, clubs());
                outgoing(4, c.r(13) + ((club_id * 0x248) + 0x221));
                outgoing(5, competition_long_name());
                outgoing(6, outlet_text);
                outgoing(7, home_short());
                outgoing(8, nation() + 0xe1);
                say(body, phrase(0x32a30), 0x10691ca0, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            } else {
                outgoing(0, home_marker());
                outgoing(1, home_marker());
                outgoing(2, nation_marker());
                c.set(13, nation());
                outgoing(3, sb(u8(c.r(13) + 0x116)));
                c.set(13, clubs());
                outgoing(4, c.r(13) + ((club_id * 0x248) + 0x221));
                outgoing(5, competition_long_name());
                outgoing(6, outlet_text);
                outgoing(7, home_short());
                outgoing(8, nation() + 0xe1);
                say(body, phrase(0x328bf), 0x10691b68, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), sb(outlet_flag()), sb(outlet_flag())});
            }
        } else {
            if (cmp(staff_id, -1) <= 0 || cmp(field19, -1) <= 0 || cmp(field1a, -1) <= 0) {
                if (cmp(staff_id, -1) <= 0 || cmp(sb(c.r(13)), 0) == 0) {
                    if (cmp(sb(c.r(16)), 0) != 0) {
                        if (cmp(field15, 0) == 0) {
                            say(body, phrase(0x323db), 0x10691600, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                        } else {
                            say(body, phrase(0x3233f), 0x1069157c, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
                        }
                    } else {
                        if (cmp(field15, 0) == 0) {
                            outgoing(0, nation() + 0xe1);
                            outgoing(1, home_short());
                            outgoing(2, competition_name());
                            say(body, phrase(0x3252c), 0x106917b0, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_marker(), competition_marker()});
                        } else {
                            outgoing(0, nation() + 0xe1);
                            outgoing(1, home_short());
                            outgoing(2, competition_name());
                            say(body, phrase(0x32474), 0x106916dc, {nation_marker(), nation_marker(), home_marker(), home_marker(), competition_marker(), competition_marker()});
                        }
                    }
                } else {
                    if (cmp(field15, 0) == 0) {
                        outgoing(0, competition_name());
                        say(body, phrase(0x32289), 0x106914e8, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                    } else {
                        outgoing(0, competition_name());
                        say(body, phrase(0x321d0), 0x1069145c, {home_marker(), home_marker(), competition_marker(), competition_marker(), subject_full, home_short()});
                    }
                }
            } else {
                call(0x100c6bf0, 0x106911d8, {{3, sb(field1a)}, {4, 0}, {5, 0}, {6, at(0x74)}});
                c.set(0, c.r(3));
                if (cmp(field15, 0) == 0) {
                    outgoing(0, competition_marker());
                    outgoing(1, competition_marker());
                    outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                    outgoing(3, c.r(0));
                    outgoing(4, subject_full);
                    outgoing(5, home_short());
                    outgoing(6, competition_name());
                    say(body, phrase(0x320fe), 0x106913b8, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x74))), sb(u8(at(0x74))), home_marker(), home_marker()});
                } else {
                    outgoing(0, competition_marker());
                    outgoing(1, competition_marker());
                    outgoing(2, clubs() + ((field19 * 0x248) + 0x221));
                    outgoing(3, c.r(3));
                    outgoing(4, subject_full);
                    outgoing(5, home_short());
                    outgoing(6, competition_name());
                    say(body, phrase(0x32029), 0x106912cc, {sb(u8(club_row(field19) + 0x23b)), sb(u8(club_row(field19) + 0x23b)), sb(u8(at(0x74))), sb(u8(at(0x74))), home_marker(), home_marker()});
                }
            }
        }
    }
    if (cmp(sb(has_match), 0) == 0 || cmp(home_goals(), away_goals()) <= 0) {
        return;
    } else {
        score(0x10691d20);
        if (cmp(staff_id, -1) <= 0 || cmp(kind, 0) != 0) {
            if (cmp(rem3((kind + club_id)), 1) == 0) {
                if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                    say(quote, phrase(0x32ef3), 0x1069208c, {rival_long_marker(), rival_long_marker(), rival_long(), score_text, clubs(), clubs()});
                } else {
                    if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                        say(quote, phrase(0x3300a), 0x10692160, {rival_long_marker(), rival_long_marker(), rival_long(), score_text, clubs(), clubs()});
                    } else {
                        say(quote, phrase(0x32f73), 0x10692104, {rival_long_marker(), rival_long_marker(), rival_long(), score_text, clubs(), clubs()});
                    }
                }
            } else {
                if (cmp(rem3((kind + club_id)), 1) >= 0 || cmp(rem3((kind + club_id)), 0) < 0) {
                    if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                        say(quote, phrase(0x3308a), 0x106921d8, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
                    } else {
                        if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                            say(quote, phrase(0x3318c), 0x106922ac, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
                        } else {
                            say(quote, phrase(0x3310d), 0x10692250, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
                        }
                    }
                } else {
                    if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                        say(quote, phrase(0x32d3a), 0x10691f40, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
                    } else {
                        if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                            say(quote, phrase(0x32e67), 0x10692014, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
                        } else {
                            say(quote, phrase(0x32dd1), 0x10691fb8, {rival_long_marker(), rival_long_marker(), score_text, rival_long(), clubs(), clubs()});
                        }
                    }
                }
            }
        } else {
            if (cmp((home_goals()) - (away_goals()), 3) > 0) {
                say(quote, phrase(0x32b97), 0x10691dac, {0x64, rival_long_marker(), rival_long_marker(), subject_short, rival_long(), score_text});
            } else {
                if (cmp((home_goals()) + (away_goals()), 5) <= 0) {
                    say(quote, phrase(0x32cb6), 0x10691e90, {0x64, rival_long_marker(), rival_long_marker(), subject_short, rival_long(), score_text});
                } else {
                    say(quote, phrase(0x32c1b), 0x10691e2c, {0x64, rival_long_marker(), rival_long_marker(), subject_short, rival_long(), score_text});
                }
            }
        }
    }
    call(0x1077dc60, 0x106922b8, {{3, body}, {4, quote}});
}

void Story::story_5b() {
    // Story code 0x5b: a club spends on new signings and the media expect more.
    put32(at(0x88), field18);
    call(0x10009f20, 0x106922d8, {{3, at(0x88)}, {4, 6}});
    call(0x1000b410, 0x106922f0, {{3, amount_text}, {4, 0x32}, {5, c.r(3)}, {6, c.r(4)}, {7, 6}});
    if (cmp(kind, 1) == 0) {
        if (cmp(staff_id, -1) <= 0) {
            say(body, phrase(0x3363e), 0x10692610, {home_marker(), home_marker(), home_short(), amount_text, clubs()});
            return;
        }
        say(body, phrase(0x33540), 0x106925c0, {home_marker(), home_marker(), home_short(), other_full, amount_text, subject_full});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(field15, 0) == 0) {
            if (cmp(staff_id, -1) <= 0) {
                outgoing(0, competition_long_name());
                say(body, phrase(0x33484), 0x10692560, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), amount_text, home_short()});
                return;
            }
            outgoing(0, home_short());
            outgoing(1, competition_long_name());
            say(body, phrase(0x3339e), 0x106924d4, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), amount_text, subject_full});
            return;
        }
        if (cmp(staff_id, -1) <= 0) {
            outgoing(0, competition_long_name());
            say(body, phrase(0x332e9), 0x10692438, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), amount_text, home_short()});
            return;
        }
        outgoing(0, home_short());
        outgoing(1, competition_long_name());
        say(body, phrase(0x33201), 0x106923ac, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), amount_text, subject_full});
        return;
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        if (cmp(staff_id, -1) <= 0) {
            say(body, phrase(0x337ac), 0x106926c4, {home_marker(), home_marker(), home_short(), amount_text, clubs()});
            return;
        }
        say(body, phrase(0x336d1), 0x10692674, {home_marker(), home_marker(), home_short(), amount_text, subject_full});
        return;
    }
    call(0x10168a00, 0x106926e8, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    if (cmp(staff_id, -1) <= 0) {
        outgoing(0, amount_text);
        say(body, phrase(0x3395c), 0x10692824, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
        return;
    }
    outgoing(0, amount_text);
    outgoing(1, subject_full);
    say(body, phrase(0x33861), 0x1069278c, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
}

void Story::story_52() {
    // Story code 0x52: a club accused by the press, who expect it to struggle.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x33ba7), 0x1069297c, {home_marker(), home_marker(), home_short(), subject_full, subject_short});
            return;
        }
        outgoing(0, subject_full);
        outgoing(1, subject_short);
        say(body, phrase(0x33cca), 0x10692a1c, {nation_marker(), nation_marker(), home_marker(), home_marker(), nation() + 0xe1, home_short()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x33a2d), 0x106928a0, {home_marker(), home_marker(), home_short(), subject_full, clubs()});
            return;
        }
        call(0x10168a00, 0x106928c4, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        outgoing(0, subject_full);
        say(body, phrase(0x33ad9), 0x10692920, {home_marker(), home_marker(), 0x64, 0x64, home_short(), nationality});
        return;
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        say(body, phrase(0x33e09), 0x10692a78, {home_marker(), home_marker(), subject_full, home_short(), subject_short});
        return;
    }
    call(0x10168a00, 0x10692a9c, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1e}});
    outgoing(0, nationality);
    outgoing(1, subject_short);
    say(body, phrase(0x33f75), 0x10692b00, {home_marker(), home_marker(), 0x64, 0x64, subject_full, home_short()});
}

void Story::story_3e() {
    // Story code 0x3e: a club deep in the relegation zone.
    if (cmp(kind, 1) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x3432f), 0x10692cf8, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            return;
        }
        call(0x10168a00, 0x10692d1c, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1f}});
        outgoing(0, nationality);
        outgoing(1, home_short());
        outgoing(2, competition_name());
        say(body, phrase(0x34407), 0x10692db4, {0x64, 0x64, home_marker(), home_marker(), competition_marker(), competition_marker()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x34103), 0x10692bb0, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
            return;
        }
        call(0x10168a00, 0x10692bd4, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
        outgoing(0, home_short());
        outgoing(1, competition_name());
        outgoing(2, nationality);
        say(body, phrase(0x34208), 0x10692c6c, {home_marker(), home_marker(), competition_marker(), competition_marker(), 0x64, 0x64});
        return;
    }
    say(body, phrase(0x34501), 0x10692e38, {home_marker(), home_marker(), competition_marker(), competition_marker(), home_short(), competition_name()});
}

void Story::story_5e() {
    // Story code 0x5e: the press run out of superlatives for a winning club.
    if (cmp(kind, 2) == 0) {
        if (cmp(sb(c.r(16)), 0) != 0) {
            say(body, phrase(0x34bf7), 0x10693294, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
            return;
        }
        outgoing(0, home_short());
        outgoing(1, competition_long_name());
        outgoing(2, nation() + 0xe1);
        say(body, phrase(0x34cbe), 0x10693368, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), nation_marker(), nation_marker()});
        return;
    }
    if (cmp(kind, 2) < 0) {
        if (cmp(kind, 0) != 0) {
            if (cmp(kind, 0) < 0) {
            } else {
                if (cmp(field15, 0) == 0) {
                    outgoing(0, competition_long_name());
                    outgoing(1, other_full);
                    say(body, phrase(0x34ab8), 0x10693208, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), 0x64, home_short()});
                    return;
                }
                outgoing(0, competition_long_name());
                outgoing(1, other_full);
                say(body, phrase(0x3496f), 0x10693174, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), 0x64, home_short()});
                return;
            }
        } else {
            if (cmp(sb(c.r(16)), 0) != 0) {
                if (cmp(field15, 0) == 0) {
                    say(body, phrase(0x346cf), 0x10692f78, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                    return;
                }
                say(body, phrase(0x345f6), 0x10692ef4, {home_marker(), home_marker(), competition_long_marker(), competition_long_marker(), home_short(), competition_long_name()});
                return;
            }
            call(0x10168a00, 0x10692f9c, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
            if (cmp(field15, 0) == 0) {
                outgoing(0, nationality);
                outgoing(1, home_short());
                outgoing(2, competition_long_name());
                say(body, phrase(0x3488a), 0x106930d8, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
                return;
            }
            outgoing(0, nationality);
            outgoing(1, home_short());
            outgoing(2, competition_long_name());
            say(body, phrase(0x34792), 0x1069303c, {0x64, 0x64, home_marker(), home_marker(), competition_long_marker(), competition_long_marker()});
            return;
        }
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        if (cmp(field15, 0) == 0) {
            say(body, phrase(0x34e37), 0x10693410, {home_marker(), home_marker(), home_short(), clubs()});
            return;
        }
        say(body, phrase(0x34d97), 0x106933c4, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    call(0x10168a00, 0x10693434, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x1c}});
    if (cmp(field15, 0) == 0) {
        say(body, phrase(0x34f97), 0x106934e8, {home_marker(), home_marker(), 0x64, 0x64, home_short(), nationality});
        return;
    }
    say(body, phrase(0x34ed1), 0x10693490, {home_marker(), home_marker(), 0x64, 0x64, home_short(), nationality});
}

void Story::story_3c() {
    // Story code 0x3c: supporters losing patience.
    if (cmp(kind, 1) == 0) {
        say(body, phrase(0x350fb), 0x106935a0, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    if (cmp(kind, 1) < 0 && cmp(kind, 0) >= 0) {
        say(body, phrase(0x35053), 0x10693554, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    if (cmp(sb(c.r(16)), 0) != 0) {
        say(body, phrase(0x35222), 0x106935f4, {home_marker(), home_marker(), home_short(), clubs()});
        return;
    }
    call(0x10168a00, 0x10693618, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x17}});
    say(body, phrase(0x3531e), 0x1069366c, {home_marker(), home_marker(), 0x64, 0x64, home_short(), nationality});
}
}
