// The rest of the text: the outlet's name, the manager's quote, the headline and the links.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

// The outlet's wording for the kind: the nationality adjective of the club's country unless the story is told at home (r16).
void Story::outlet_name() {
    write_outlet(c, {0x1068a9d8, outlet_text, nationality, at(0x87)}, std::int32_t(outlet_kind), c.r(16), home());
}

namespace {
// The user's own club's story, when the manager agrees (mood 2).
constexpr QuoteEntry own_agreed[]{
    {0x3c, 0x391e7, 0x10697228}, {0x3d, 0x39243, 0x10697244}, {0x5d, 0x392e8, 0x10697260}, {0x63, 0x39338, 0x1069727c},
    {0x62, 0x3937c, 0x10697298}, {0x61, 0x39338, 0x106972b4}, {0x40, 0x393c6, 0x106972d0}, {0x41, 0x39411, 0x106972ec},
    {0x3f, 0x39462, 0x10697308}, {0x5f, 0x394ae, 0x10697324}, {0x60, 0x394f3, 0x10697340}, {0x42, 0x3952e, 0x1069735c},
    {0x43, 0x3956c, 0x10697378}, {0x44, 0x3959d, 0x10697394}, {0x5a, 0x395df, 0x106973b0},
    {0x5c, 0x3961e, 0x10697424, Test::field15, 0x39686, 0x10697488, true}, {0x5b, 0x396e5, 0x106974a4}};
// The same when the mood is between 0 and 2.
constexpr QuoteEntry own_mild[]{
    {0x3c, 0x39778, 0x106974f4}, {0x3d, 0x397e0, 0x10697510}, {0x5d, 0x39835, 0x1069752c}, {0x63, 0x39879, 0x10697548},
    {0x62, 0x398f0, 0x1069756c, Test::field15, 0x39932, 0x10697580}, {0x61, 0x39972, 0x1069759c}, {0x40, 0x399b7, 0x106975b8},
    {0x41, 0x399b7, 0x106975d4}, {0x3f, 0x399f9, 0x106975f0}, {0x5f, 0x39a56, 0x1069760c}, {0x60, 0x39aa3, 0x10697628},
    {0x42, 0x39af4, 0x10697644}, {0x43, 0x39b3c, 0x10697660}, {0x44, 0x39b80, 0x1069767c}, {0x5a, 0x39bc4, 0x10697698},
    {0x5c, 0x39c0b, 0x1069770c, Test::field15, 0x39c6c, 0x10697770, true}, {0x5b, 0x39cc7, 0x1069778c}};
// Another club's story, which the other person (in `other_full`) tells: mood 2, then between 0 and 2.
constexpr QuoteEntry other_agreed[]{
    {0x3c, 0x3a818, 0x10697c00}, {0x3d, 0x3a897, 0x10697c20}, {0x5d, 0x3a965, 0x10697c40},
    {0x63, 0x3a9d2, 0x10697c6c, Test::field5, 0x3aa59, 0x10697c84}, {0x62, 0x3a9d2, 0x10697ca4}, {0x61, 0x3aaf5, 0x10697cc4},
    {0x40, 0x3ab62, 0x10697ce4}, {0x41, 0x3ab62, 0x10697d04}, {0x3f, 0x3abc6, 0x10697d24}, {0x5f, 0x3ac5d, 0x10697d44},
    {0x60, 0x3acd9, 0x10697d64}, {0x42, 0x3ad4e, 0x10697d84}, {0x43, 0x3adba, 0x10697da4}, {0x44, 0x3ae23, 0x10697dc4},
    {0x5a, 0x3ae91, 0x10697df0, Test::field5, 0x3af0a, 0x10697e08},
    {0x5c, 0x3af9a, 0x10697e80, Test::field15, 0x3b01b, 0x10697ee8, true}, {0x5b, 0x3b09b, 0x10697f08}};
constexpr QuoteEntry other_mild_first[]{{0x3c, 0x39eb5, 0x106978a0}, {0x3d, 0x39f4b, 0x106978c0}, {0x5d, 0x39fcb, 0x106978e0}};
constexpr QuoteEntry other_mild_rest[]{
    {0x62, 0x3a19c, 0x10697964}, {0x61, 0x3a20c, 0x10697984}, {0x40, 0x3a275, 0x106979a4}, {0x41, 0x3a2e5, 0x106979c4},
    {0x3f, 0x3a35f, 0x106979e4}, {0x5f, 0x3a3cf, 0x10697a04}, {0x60, 0x3a43e, 0x10697a24}, {0x42, 0x3a49e, 0x10697a44},
    {0x43, 0x3a500, 0x10697a64}, {0x44, 0x3a556, 0x10697a84}, {0x5a, 0x3a5bc, 0x10697aa4},
    {0x5c, 0x3a625, 0x10697b1c, Test::field15, 0x3a6b6, 0x10697b84, true}, {0x5b, 0x3a73e, 0x10697ba4}};

constexpr HeadlineEntry headline_opening[]{
    {0x50, 0x3b1a0, 0x106985a8, Words::club_and_row}, {0x51, 0x3b1cb, 0x106985d8}, {0x3f, 0x3b1f9, 0x10698608},
    {0x5f, 0x3b228, 0x10698638}, {0x60, 0x3b261, 0x10698668}};
constexpr HeadlineEntry headline_43[]{{0x43, 0x3b332, 0x1069870c}};
constexpr HeadlineEntry headline_accused[]{
    {0x52, 0x3b471, 0x10698888, Words::subject}, {0x5b, 0x3b4a6, 0x106988b8}, {0x3e, 0x3b4e0, 0x106988e8},
    {0x5e, 0x3b514, 0x10698918}, {0x3c, 0x3b541, 0x10698948}, {0x3d, 0x3b573, 0x10698978}};
constexpr HeadlineEntry headline_doubtful[]{{0x62, 0x3b679, 0x10698a70}, {0x61, 0x3b6a4, 0x10698aa0}, {0x40, 0x3b6cd, 0x10698ad0}};
}

// Says the quote of the first entry that matches the story code: with the other person's name when
// `with_other`. Returns whether one did.
bool Story::say_quote(std::span<const QuoteEntry> table, bool with_other) {
    for (const auto& entry : table) {
        if (cmp(code, entry.code) != 0) continue;
        auto text = entry.phrase;
        auto back = entry.back;
        if (entry.test != Test::none && cmp(entry.test == Test::field5 ? field5 : field15, 0) == 0) {
            text = entry.zero_phrase;
            back = entry.zero_back;
        }
        if (entry.nation) {
            // The nation's region: its marker byte twice, then (after the other person's name, if any) its text and itself.
            const auto region = [&] { return u32(nation() + 0x74); };
            const auto marker = [&] { return sb(u8(region() + 0xc3)); };
            if (with_other) say(quote, phrase(text), back, {marker(), marker(), other_full, region() + 0x10, region()});
            else say(quote, phrase(text), back, {marker(), marker(), region() + 0x10, region()});
        } else if (with_other) say(quote, phrase(text), back, {other_full});
        else say(quote, phrase(text), back, {});
        return true;
    }
    return false;
}

bool Story::say_headline(std::span<const HeadlineEntry> table) {
    for (const auto& entry : table) {
        if (cmp(code, entry.code) != 0) continue;
        switch (entry.words) {
        case Words::club: say(body, phrase(entry.phrase), entry.back, {home_marker(), home_marker(), home_short()}); break;
        case Words::club_and_row: say(body, phrase(entry.phrase), entry.back, {home_marker(), home_marker(), home_short(), home()}); break;
        case Words::subject: say(body, phrase(entry.phrase), entry.back, {subject_short}); break;
        }
        return true;
    }
    return false;
}

// The manager's quote. Whose club it is (the user's or another), and the mood, choose the ladder of story
// codes; a code that is not on the ladder has the plain text.
void Story::quote_text() {
    const auto plain = [&](std::uint32_t back) { call(0x1077b720, back, {{3, quote}, {4, phrase(0x9f58)}}); };
    if (cmp(sb(own_club), 0) != 0) {
        if (cmp(mood, 2) == 0) {
            if (say_quote(own_agreed, false)) return;
            if (cmp(code, 0x52) != 0) return say(quote, phrase(0x9f58), 0x106974d8, {});
            return say(quote, phrase(0x3971a), 0x106974c4, {subject_short});
        }
        if (cmp(mood, 2) >= 0) {
            if (cmp(mood, 4) >= 0) return plain(0x1069784c);
            if (cmp(code, 0x3d) != 0 || cmp(rem3(club_id), 0) != 0) return say(quote, phrase(0xa7c4), 0x10697838, {});
            if (cmp(rem4(club_id), 0) == 0) return say(quote, phrase(0x39e08), 0x10697824, {});
            return say(quote, phrase(0x39d59), 0x10697810, {});
        }
        if (cmp(mood, 0) == 0 || cmp(mood, 0) < 0) return plain(0x1069784c);
        if (say_quote(own_mild, false)) return;
        if (cmp(code, 0x52) != 0) return say(quote, phrase(0x9f58), 0x106977bc, {});
        return say(quote, phrase(0x39d16), 0x106977a8, {});
    }
    if (cmp(mood, 2) == 0) {
        if (say_quote(other_agreed, true)) return;
        if (cmp(code, 0x52) != 0) return plain(0x10697f40);
        return say(quote, phrase(0x3b11b), 0x10697f2c, {other_full, subject_short});
    }
    if (cmp(mood, 2) >= 0) {
        if (cmp(mood, 4) < 0) {
            if (cmp(code, 0x3d) != 0 || cmp(rem3(club_id), 0) != 0) return plain(0x10697fbc);
            if (cmp(rem4(club_id), 0) == 0) return say(quote, phrase(0x39e08), 0x10697fa8, {});
            return say(quote, phrase(0x39d59), 0x10697f94, {});
        }
    } else if (cmp(mood, 0) != 0 && cmp(mood, 0) >= 0) {
        if (say_quote(other_mild_first, true)) return;
        if (cmp(code, 0x63) == 0) {
            if (cmp(field5, 0) != 0) return say(quote, phrase(0x3a041), 0x1069790c, {other_full});
            if (cmp(field6, 0) == 0) return say(quote, phrase(0x9f58), 0x10697944, {});
            return say(quote, phrase(0x3a0fc), 0x10697930, {other_full});
        }
        if (say_quote(other_mild_rest, true)) return;
        if (cmp(code, 0x52) != 0) return plain(0x10697be0);
        return say(quote, phrase(0x3a798), 0x10697bcc, {0x64, other_full, subject_short});
    }
    plain(0x10697fd0);
}

void Story::headline() {
    if (say_headline(headline_opening)) return;
    if (cmp(code, 0x42) == 0 || cmp(code, 0x44) == 0) {
        if (cmp(kind, 0) != 0 || cmp(staff_id, -1) <= 0) return say(body, phrase(0x3b2f6), 0x106986dc, {home_marker(), home_marker(), home_short()});
        return say(body, phrase(0x3b298), 0x106986b4, {home_marker(), home_marker(), subject_short, home_short()});
    }
    if (say_headline(headline_43)) return;
    if (cmp(code, 0x5a) == 0) {
        if (cmp(rem2((club_id + (code * kind))), 0) == 0) return say(body, phrase(0x3b35d), 0x1069875c, {home_marker(), home_marker(), home_short()});
        return say(body, phrase(0x3b388), 0x10698784, {home_marker(), home_marker(), home_short()});
    }
    if (cmp(code, 0x5c) == 0) {
        if (cmp(rem3((kind + club_id)), 1) == 0) return say(body, phrase(0x3b40b), 0x10698840, {home_marker(), home_marker(), home_short()});
        if (cmp(rem3((kind + club_id)), 1) < 0 && cmp(rem3((kind + club_id)), 0) >= 0) {
            const auto region = [&] { return u32(nation() + 0x74); };
            return say(body, phrase(0x3b3b5), 0x10698818, {home_marker(), home_marker(), sb(u8(region() + 0xc3)), sb(u8(region() + 0xc3)), home_short(), region() + 0x10});
        }
    } else {
        if (say_headline(headline_accused)) return;
        if (cmp(code, 0x63) == 0) {
            if (cmp(kind, 0) == 0) return say(body, phrase(0x3b5a7), 0x106989b4, {home_marker(), home_marker(), home_short()});
            return say(body, phrase(0x3b5d9), 0x106989dc, {home_marker(), home_marker(), home_short()});
        }
        if (cmp(code, 0x5d) == 0) {
            if (cmp(staff_id, -1) != 0) return say(body, phrase(0x3b637), 0x10698a40, {home_marker(), home_marker(), subject_short, home_short()});
            return say(body, phrase(0x3b60e), 0x10698a14, {home_marker(), home_marker(), home_short()});
        }
        if (say_headline(headline_doubtful)) return;
        if (cmp(code, 0x41) != 0) return say(body, phrase(0x3b770), 0x10698b5c, {home_marker(), home_marker(), home_short()});
        if (cmp(staff_id, -1) != 0) return say(body, phrase(0x3b72a), 0x10698b34, {home_marker(), home_marker(), subject_short, home_short()});
        return say(body, phrase(0x3b6fd), 0x10698b08, {home_marker(), home_marker(), home_short()});
    }
    say(body, phrase(0x3b440), 0x10698868, {home_marker(), home_marker(), home_short()});
}

void Story::links() {
    call(0x1077b720, 0x10697fe8, {{3, joined}, {4, phrase(0xc024)}, {5, body}, {6, quote}});
    call(0x1074b830, 0x10697ff4, {{3, output}, {4, joined}});
    if (cmp(staff_id, -1) > 0) {
        call(0x100cbde0, 0x10698020, {{3, staff_row()}, {4, staff_first}, {5, staff_middle}, {6, staff_last}, {7, 0}});
        if (cmp(sb(c.r(3)), 0) != 0) {
            call(0x1074bab0, 0x10698048, {{3, output}, {4, subject_full}, {5, 0x7d7}, {6, staff_row()}});
        }
    }
    call(0x100cbde0, 0x10698070, {{3, other_row()}, {4, other_first}, {5, other_middle}, {6, other_last}, {7, 0}});
    if (cmp(sb(c.r(3)), 0) != 0) {
        call(0x1074bab0, 0x1069809c, {{3, output}, {4, other_full}, {5, 0x7d7}, {6, other_row()}});
    }
    if (cmp(sb(has_match), 0) != 0) {
        call(0x1029eb20, 0x106980cc, {{3, u32(toc(-0x7b4c))}, {4, u32(at(0xec))}, {5, u32(at(0xe0))}, {6, rival_id()}, {7, i16(at(0xcc)) & 0xffff}, {8, u16(at(0xce))}});
        if (cmp(sb(c.r(3)), 0) == 0) {
            call(0x1074bab0, 0x10698118, {{3, output}, {4, rival_long()}, {5, 0x7d5}, {6, rival()}, {7, clubs()}});
        } else {
            call(0x1074bab0, 0x106980e8, {{3, output}, {4, score_text}, {5, 0x7d3}, {6, u32(at(0xec))}});
            call(0x1074bab0, 0x10698118, {{3, output}, {4, rival_long()}, {5, 0x7d5}, {6, rival()}, {7, clubs()}});
        }
    }
    if (cmp(staff_id, -1) > 0) {
        call(0x100cbde0, 0x10698144, {{3, staff_row()}, {4, staff_first}, {5, staff_middle}, {6, staff_last}, {7, 0}});
        if (cmp(sb(c.r(3)), 0) != 0) {
            call(0x1074bab0, 0x1069816c, {{3, output}, {4, subject_full}, {5, 0x7d7}, {6, staff_row()}});
            call(0x1074bab0, 0x1069818c, {{3, output}, {4, subject_short}, {5, 0x7d7}, {6, staff_row()}});
        }
    }
    call(0x1074bab0, 0x106981b4, {{3, output}, {4, competition_long_name()}, {5, 0x7d0}, {6, competition()}, {7, competitions()}});
    call(0x1074bab0, 0x106981dc, {{3, output}, {4, competition_name()}, {5, 0x7d0}, {6, competition()}, {7, competitions()}});
    call(0x1074bab0, 0x10698204, {{3, output}, {4, home_short()}, {5, 0x7d5}, {6, home()}, {7, clubs()}});
    call(0x1074bab0, 0x1069822c, {{3, output}, {4, home_long()}, {5, 0x7d5}, {6, home()}, {7, clubs()}});
    call(0x100c76c0, 0x10698240, {{3, nation()}});
    call(0x1074bab0, 0x10698264, {{3, output}, {4, nation() + 0xae}, {5, 0x7d5}, {6, c.r(3)}});
    call(0x100c76c0, 0x10698278, {{3, nation()}});
    call(0x1074bab0, 0x1069829c, {{3, output}, {4, nation() + 0xe1}, {5, 0x7d5}, {6, c.r(3)}});
    // One name per outlet kind of the club's country, each linked: the nationality adjective for kinds 0x16 to 0x1f.
    for (unsigned kind_index = 0; kind_index < 10; ++kind_index) {
        const auto back = 0x106982bc + 0x48 * kind_index;
        call(0x10168a00, back, {{3, nation()}, {4, nationality}, {5, 0x78}, {6, 0x16 + kind_index}});
        call(0x100c76c0, back + 0x14, {{3, nation()}});
        call(0x1074bab0, back + 0x28, {{3, output}, {4, nationality}, {5, 0x7d5}, {6, c.r(3)}});
    }
}
}
