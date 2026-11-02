// The rest of the text: the outlet's name, the manager's quote, the headline and the links.
#include "media_text_d_detail.hpp"

namespace cm::aot::media::detail::story {
using namespace guest;
using namespace detail;

// The outlet's wording for the kind: the nationality adjective of the club's country unless the story is told at home (`domestic`).
void Story::outlet_name() {
    write_outlet(c, {0x1068a9d8, outlet_text, nationality, at(0x87)}, std::int32_t(outlet_kind), domestic, home());
}

namespace {
// The user's own club's story, when the user's manager agrees (mood 2).
constexpr QuoteEntry users_agreed[]{
    {0x3c, 0x391e7, 0x10697228}, {0x3d, 0x39243, 0x10697244}, {0x5d, 0x392e8, 0x10697260}, {0x63, 0x39338, 0x1069727c},
    {0x62, 0x3937c, 0x10697298}, {0x61, 0x39338, 0x106972b4}, {0x40, 0x393c6, 0x106972d0}, {0x41, 0x39411, 0x106972ec},
    {0x3f, 0x39462, 0x10697308}, {0x5f, 0x394ae, 0x10697324}, {0x60, 0x394f3, 0x10697340}, {0x42, 0x3952e, 0x1069735c},
    {0x43, 0x3956c, 0x10697378}, {0x44, 0x3959d, 0x10697394}, {0x5a, 0x395df, 0x106973b0},
    {0x5c, 0x3961e, 0x10697424, Test::title_holder, 0x39686, 0x10697488, true}, {0x5b, 0x396e5, 0x106974a4}};
// The same when the mood is between 0 and 2.
constexpr QuoteEntry users_mild[]{
    {0x3c, 0x39778, 0x106974f4}, {0x3d, 0x397e0, 0x10697510}, {0x5d, 0x39835, 0x1069752c}, {0x63, 0x39879, 0x10697548},
    {0x62, 0x398f0, 0x1069756c, Test::title_holder, 0x39932, 0x10697580}, {0x61, 0x39972, 0x1069759c}, {0x40, 0x399b7, 0x106975b8},
    {0x41, 0x399b7, 0x106975d4}, {0x3f, 0x399f9, 0x106975f0}, {0x5f, 0x39a56, 0x1069760c}, {0x60, 0x39aa3, 0x10697628},
    {0x42, 0x39af4, 0x10697644}, {0x43, 0x39b3c, 0x10697660}, {0x44, 0x39b80, 0x1069767c}, {0x5a, 0x39bc4, 0x10697698},
    {0x5c, 0x39c0b, 0x1069770c, Test::title_holder, 0x39c6c, 0x10697770, true}, {0x5b, 0x39cc7, 0x1069778c}};
// Another club's story, in which the club's manager (`manager_full`) is quoted: mood 2, then between 0 and 2.
constexpr QuoteEntry manager_agreed[]{
    {0x3c, 0x3a818, 0x10697c00}, {0x3d, 0x3a897, 0x10697c20}, {0x5d, 0x3a965, 0x10697c40},
    {0x63, 0x3a9d2, 0x10697c6c, Test::field5, 0x3aa59, 0x10697c84}, {0x62, 0x3a9d2, 0x10697ca4}, {0x61, 0x3aaf5, 0x10697cc4},
    {0x40, 0x3ab62, 0x10697ce4}, {0x41, 0x3ab62, 0x10697d04}, {0x3f, 0x3abc6, 0x10697d24}, {0x5f, 0x3ac5d, 0x10697d44},
    {0x60, 0x3acd9, 0x10697d64}, {0x42, 0x3ad4e, 0x10697d84}, {0x43, 0x3adba, 0x10697da4}, {0x44, 0x3ae23, 0x10697dc4},
    {0x5a, 0x3ae91, 0x10697df0, Test::field5, 0x3af0a, 0x10697e08},
    {0x5c, 0x3af9a, 0x10697e80, Test::title_holder, 0x3b01b, 0x10697ee8, true}, {0x5b, 0x3b09b, 0x10697f08}};
constexpr QuoteEntry manager_mild_first[]{{0x3c, 0x39eb5, 0x106978a0}, {0x3d, 0x39f4b, 0x106978c0}, {0x5d, 0x39fcb, 0x106978e0}};
constexpr QuoteEntry manager_mild_rest[]{
    {0x62, 0x3a19c, 0x10697964}, {0x61, 0x3a20c, 0x10697984}, {0x40, 0x3a275, 0x106979a4}, {0x41, 0x3a2e5, 0x106979c4},
    {0x3f, 0x3a35f, 0x106979e4}, {0x5f, 0x3a3cf, 0x10697a04}, {0x60, 0x3a43e, 0x10697a24}, {0x42, 0x3a49e, 0x10697a44},
    {0x43, 0x3a500, 0x10697a64}, {0x44, 0x3a556, 0x10697a84}, {0x5a, 0x3a5bc, 0x10697aa4},
    {0x5c, 0x3a625, 0x10697b1c, Test::title_holder, 0x3a6b6, 0x10697b84, true}, {0x5b, 0x3a73e, 0x10697ba4}};

constexpr HeadlineEntry headline_opening[]{
    {0x50, 0x3b1a0, 0x106985a8, Words::club_and_row}, {0x51, 0x3b1cb, 0x106985d8}, {0x3f, 0x3b1f9, 0x10698608},
    {0x5f, 0x3b228, 0x10698638}, {0x60, 0x3b261, 0x10698668}};
constexpr HeadlineEntry headline_43[]{{0x43, 0x3b332, 0x1069870c}};
constexpr HeadlineEntry headline_accused[]{
    {0x52, 0x3b471, 0x10698888, Words::subject}, {0x5b, 0x3b4a6, 0x106988b8}, {0x3e, 0x3b4e0, 0x106988e8},
    {0x5e, 0x3b514, 0x10698918}, {0x3c, 0x3b541, 0x10698948}, {0x3d, 0x3b573, 0x10698978}};
constexpr HeadlineEntry headline_doubtful[]{{0x62, 0x3b679, 0x10698a70}, {0x61, 0x3b6a4, 0x10698aa0}, {0x40, 0x3b6cd, 0x10698ad0}};
}

// Says the quote of the first entry that matches the story code: with the manager's name when
// `with_manager`. Returns whether one did.
bool Story::say_quote(std::span<const QuoteEntry> table, bool with_manager) {
    for (const auto& entry : table) {
        if (cmp(code, entry.code) != 0) continue;
        auto text = entry.phrase;
        auto back = entry.back;
        if (entry.test != Test::none && cmp(entry.test == Test::field5 ? field5 : title_holder, 0) == 0) {
            text = entry.zero_phrase;
            back = entry.zero_back;
        }
        if (entry.nation) {
            // The nation's continent: its marker byte twice, then (after the manager's name, if any) its text and itself.
            if (with_manager) say(quote, phrase(text), back, {continent_marker(), continent_marker(), manager_full, continent_name(), continent()});
            else say(quote, phrase(text), back, {continent_marker(), continent_marker(), continent_name(), continent()});
        } else if (with_manager) say(quote, phrase(text), back, {manager_full});
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
        default: say(body, phrase(entry.phrase), entry.back, {subject_short}); break;  // Words::subject
        }
        return true;
    }
    return false;
}

// The manager's quote. Whose club it is (the user's or another), and the mood, choose the ladder of story
// codes; a code that is not on the ladder has the plain text.
void Story::quote_text() {
    const auto plain = [&](std::uint32_t back) { call(sprintf_entry, back, {{3, quote}, {4, phrase(0x9f58)}}); };
    if (cmp(sb(users_club), 0) != 0) {
        if (cmp(mood, 2) == 0) {
            if (say_quote(users_agreed, false)) return;
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
        if (say_quote(users_mild, false)) return;
        if (cmp(code, 0x52) != 0) return say(quote, phrase(0x9f58), 0x106977bc, {});
        return say(quote, phrase(0x39d16), 0x106977a8, {});
    }
    if (cmp(mood, 2) == 0) {
        if (say_quote(manager_agreed, true)) return;
        if (cmp(code, 0x52) != 0) return plain(0x10697f40);
        return say(quote, phrase(0x3b11b), 0x10697f2c, {manager_full, subject_short});
    }
    if (cmp(mood, 2) >= 0) {
        if (cmp(mood, 4) < 0) {
            if (cmp(code, 0x3d) != 0 || cmp(rem3(club_id), 0) != 0) return plain(0x10697fbc);
            if (cmp(rem4(club_id), 0) == 0) return say(quote, phrase(0x39e08), 0x10697fa8, {});
            return say(quote, phrase(0x39d59), 0x10697f94, {});
        }
    } else if (cmp(mood, 0) != 0 && cmp(mood, 0) >= 0) {
        if (say_quote(manager_mild_first, true)) return;
        if (cmp(code, 0x63) == 0) {
            if (cmp(field5, 0) != 0) return say(quote, phrase(0x3a041), 0x1069790c, {manager_full});
            if (cmp(field6, 0) == 0) return say(quote, phrase(0x9f58), 0x10697944, {});
            return say(quote, phrase(0x3a0fc), 0x10697930, {manager_full});
        }
        if (say_quote(manager_mild_rest, true)) return;
        if (cmp(code, 0x52) != 0) return plain(0x10697be0);
        return say(quote, phrase(0x3a798), 0x10697bcc, {unmarked, manager_full, subject_short});
    }
    plain(0x10697fd0);
}

void Story::headline() {
    if (say_headline(headline_opening)) return;
    if (cmp(code, 0x42) == 0 || cmp(code, 0x44) == 0) {
        if (cmp(kind, 0) != 0 || cmp(subject_id, -1) <= 0) return say(body, phrase(0x3b2f6), 0x106986dc, {home_marker(), home_marker(), home_short()});
        return say(body, phrase(0x3b298), 0x106986b4, {home_marker(), home_marker(), subject_short, home_short()});
    }
    if (say_headline(headline_43)) return;
    if (cmp(code, 0x5a) == 0) {
        if (cmp(rem2(club_id + code * kind), 0) == 0) return say(body, phrase(0x3b35d), 0x1069875c, {home_marker(), home_marker(), home_short()});
        return say(body, phrase(0x3b388), 0x10698784, {home_marker(), home_marker(), home_short()});
    }
    if (cmp(code, 0x5c) == 0) {
        const auto cycle = rem3(kind + club_id);  // (the sum wraps; the remainder is signed)
        if (cmp(cycle, 1) == 0) return say(body, phrase(0x3b40b), 0x10698840, {home_marker(), home_marker(), home_short()});
        if (cmp(cycle, 1) < 0 && cmp(cycle, 0) >= 0) {
            return say(body, phrase(0x3b3b5), 0x10698818, {home_marker(), home_marker(), continent_marker(), continent_marker(), home_short(), continent_name()});
        }
    } else {
        if (say_headline(headline_accused)) return;
        if (cmp(code, 0x63) == 0) {
            if (cmp(kind, 0) == 0) return say(body, phrase(0x3b5a7), 0x106989b4, {home_marker(), home_marker(), home_short()});
            return say(body, phrase(0x3b5d9), 0x106989dc, {home_marker(), home_marker(), home_short()});
        }
        if (cmp(code, 0x5d) == 0) {
            if (cmp(subject_id, -1) != 0) return say(body, phrase(0x3b637), 0x10698a40, {home_marker(), home_marker(), subject_short, home_short()});
            return say(body, phrase(0x3b60e), 0x10698a14, {home_marker(), home_marker(), home_short()});
        }
        if (say_headline(headline_doubtful)) return;
        if (cmp(code, 0x41) != 0) return say(body, phrase(0x3b770), 0x10698b5c, {home_marker(), home_marker(), home_short()});
        if (cmp(subject_id, -1) != 0) return say(body, phrase(0x3b72a), 0x10698b34, {home_marker(), home_marker(), subject_short, home_short()});
        return say(body, phrase(0x3b6fd), 0x10698b08, {home_marker(), home_marker(), home_short()});
    }
    say(body, phrase(0x3b440), 0x10698868, {home_marker(), home_marker(), home_short()});
}

// The body and the quote joined into the output, and the names in them made into links: the subject, the
// manager, the rival and the score, the competition, the club, the nation and the nationality adjectives.
void Story::links() {
    call(sprintf_entry, 0x10697fe8, {{3, joined}, {4, phrase(0xc024)}, {5, body}, {6, quote}});
    call(entry::set_text, 0x10697ff4, {{3, output}, {4, joined}});
    // A name is linked only when the person's staff record accepts the three parts of it (0x100cbde0).
    const auto named = [&](std::uint32_t row, std::uint32_t first, std::uint32_t middle, std::uint32_t last, std::uint32_t back) {
        call(entry::staff_named, back, {{3, row}, {4, first}, {5, middle}, {6, last}, {7, 0}});
        return cmp(sb(c.r(3)), 0) != 0;
    };
    const auto link = [&](std::uint32_t back, std::uint32_t text, std::uint32_t kind, std::uint32_t target, std::optional<std::uint32_t> table = {}) {
        if (table) call(entry::add_link, back, {{3, output}, {4, text}, {5, kind}, {6, target}, {7, *table}});
        else call(entry::add_link, back, {{3, output}, {4, text}, {5, kind}, {6, target}});
    };
    if (cmp(subject_id, -1) > 0) {
        if (named(subject_row(), subject_first, subject_middle, subject_last, 0x10698020)) link(0x10698048, subject_full, link_kind::staff, subject_row());
    }
    if (named(manager_row(), manager_first, manager_middle, manager_last, 0x10698070)) link(0x1069809c, manager_full, link_kind::staff, manager_row());
    if (cmp(sb(has_match), 0) != 0) {
        call(report::match_known, 0x106980cc, {{3, u32(toc(-0x7b4c))}, {4, u32(at(0xec))}, {5, u32(at(0xe0))}, {6, rival_id()}, {7, i16(at(0xcc)) & 0xffff}, {8, u16(at(0xce))}});
        if (cmp(sb(c.r(3)), 0) != 0) link(0x106980e8, score_text, link_score, u32(at(0xec)));
        link(0x10698118, rival_long(), link_kind::club, rival(), clubs());
    }
    if (cmp(subject_id, -1) > 0) {
        if (named(subject_row(), subject_first, subject_middle, subject_last, 0x10698144)) {
            link(0x1069816c, subject_full, link_kind::staff, subject_row());
            link(0x1069818c, subject_short, link_kind::staff, subject_row());
        }
    }
    link(0x106981b4, competition_long_name(), link_kind::competition, competition(), competitions());
    link(0x106981dc, competition_name(), link_kind::competition, competition(), competitions());
    link(0x10698204, home_short(), link_kind::club, home(), clubs());
    link(0x1069822c, home_long(), link_kind::club, home(), clubs());
    // The nation's team (0x100c76c0 finds it), linked under the given text.
    const auto link_nation_team = [&](std::uint32_t call_back, std::uint32_t link_back, std::uint32_t text) {
        call(report::nation_team, call_back, {{3, nation()}});
        link(link_back, text, link_kind::club, c.r(3));
    };
    link_nation_team(0x10698240, 0x10698264, home_nation().short_name());
    link_nation_team(0x10698278, 0x1069829c, home_nation().name());
    // One name per outlet kind of the club's country, each linked: the nationality adjective for kinds 0x16 to 0x1f.
    for (unsigned kind_index = 0; kind_index < 10; ++kind_index) {
        const auto back = 0x106982bc + 0x48 * kind_index;
        adjective_of_nation({back, 0x16 + kind_index});
        link_nation_team(back + 0x14, back + 0x28, nationality);
    }
}
}
