// DISCIPLINE.CPP news block cases (discipline_checks.hpp): 0x100e0b70, the ban story of news message type 2002.
// The message is a scratch record of words (0x102cb570 reads the word at +8+4n) the way post_ban_news writes it: the
// fifteen fields, the person (the three name indices and the table index of a person), whether a fixture follows
// and the fixture's nine words (real clubs, a real competition, scores, a record). The text object is a scratch
// object; the formatters, the name formatter and the text and link routines run for real, so the text and its link
// rows are compared byte for byte. The lookups that decide what the story names (0x100cbde0 the name link,
// 0x104abf40 and 0x104646d0 the competition, 0x1029eb20 the fixture link) are answered, and run for real in a few cases.
#include "discipline_checks.hpp"
#include <cstdint>
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t story = 0x100e0b70, name_matches = 0x100cbde0, competition_hosted = 0x104abf40,
                        competition_cup = 0x104646d0, fixture_link = 0x1029eb20;
constexpr std::uint32_t message_at = scratch_base + 0x9000, out_object = scratch_base + 0xa000, record_at = scratch_base + 0xb000;

struct Story {
    bool body = true;
    std::uint32_t extra = 0;
    std::int32_t length = 2, date = 9;
    std::uint32_t end_day = 3, end_year = 0;
    std::int32_t bodies[4] = {5, -1, -1, -1};
    std::int32_t kind = 0, count = 1, refuse_four = 0, flagged = 0;
    bool has_fixture = true;
    unsigned club_one = 5, club_two = 7, competition = 2;
    std::int32_t score_one = 3, score_two = 1;
    std::uint32_t fixture_kind = 5, fixture_group = 0;
    std::uint32_t matched = 1, link = 0, hosted = 1, cup = 0;  // hosted 0: the competition is named
    bool real_lookups = false;
    unsigned person = 100;
};

Case make(const Scenario& sc, const Story& s, std::string label) {
    Case item{.entry = story, .registers = {{3, sc.global(-0x7aa4)}, {4, message_at}, {5, s.body ? 1u : 0u}, {6, out_object}},
              .label = "news: " + std::move(label)};
    const auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    const auto field = [&](unsigned n, std::uint32_t value) { put(4, message_at + 8 + 4 * n, value); };
    put(4, message_at, 2002);
    field(0, s.extra);
    field(2, std::uint32_t(s.length));
    field(3, std::uint32_t(s.date));
    field(4, s.end_day);
    field(5, s.end_year);
    for (unsigned k = 0; k < 4; ++k) field(6 + k, std::uint32_t(s.bodies[k]) & 0xff);
    field(10, std::uint32_t(s.kind));
    field(11, std::uint32_t(s.count));
    field(12, std::uint32_t(s.refuse_four));
    field(14, std::uint32_t(s.flagged));
    // The person's three name records as table indices (the tables of 0x2c-byte text records at TOC -0x752c, -0x7530, -0x7534).
    const auto row = sc.person(s.person);
    for (unsigned k = 0; k < 3; ++k) {
        const auto record = sc.r32(row + 0x28 + 4 * k), base = sc.global(-0x752c - 4 * int(k));
        field(17 + k, record ? (record - base) / 0x2c : 0xffffffffu);
    }
    field(20, s.person);
    field(21, 50);   // the person's day and year (0x107156b0 normalises them into a date)
    field(22, 2001);
    field(23, s.has_fixture ? 1 : 0);
    // The fixture: day and year, competition, kind and group halfwords, a byte, the two clubs, their goals, the record.
    field(24, 100);
    field(25, 2001);
    field(26, s.competition);
    field(27, s.fixture_kind);
    field(28, s.fixture_group);
    field(29, 0);
    field(30, s.club_one);
    field(31, s.club_two);
    field(32, std::uint32_t(s.score_one));
    field(33, std::uint32_t(s.score_two));
    field(34, record_at);
    if (!s.real_lookups)
        item.answers = {{name_matches, s.matched}, {competition_hosted, s.hosted}, {competition_cup, s.cup}, {fixture_link, s.link}};
    return item;
}
}

DISCIPLINE_CASES("news") {
    if (scenario.match) return;
    unsigned n = 0;
    const auto add = [&](const Story& s, const char* what) { cases.push_back(make(scenario, s, std::string(what) + " " + std::to_string(n++))); };

    // ---- the headline: the ban's length (a date, one match, several) with and without the name link.
    for (std::int32_t length : {-128, -3, -1, 0, 1, 2, 5, 127})
        for (std::uint32_t matched : {0u, 1u}) {
            Story s;
            s.body = false;
            s.length = length;
            s.matched = matched;
            add(s, "headline");
        }
    {
        Story s;
        s.body = false;
        s.real_lookups = true;
        add(s, "headline, real lookups");
        s.length = -4;
        add(s, "headline, real lookups, dated");
    }

    // ---- the body of a ban: every wording (length class, kind, count, competition named, which club leads,
    // the flag and the end date).
    struct Scores { std::int32_t one, two; };
    const Scores relations[3] = {{3, 1}, {1, 3}, {2, 2}};
    for (std::int32_t length : {-2, 0, 1, 3})
        for (std::int32_t kind : {0, 1, 2, 3, 4})
            for (std::int32_t count : {1, 2})
                for (std::uint32_t hosted : {0u, 1u})
                    for (const auto& relation : relations)
                        for (unsigned arm = 0; arm < 3; ++arm) {
                            // Only some choices matter: the count for kind 1, the competition for the kinds that name it.
                            const bool names_matter = kind >= 1 && kind <= 3 && (kind != 1 || count == 1);
                            if ((kind != 1 && count != 1) || (!names_matter && hosted == 0)) continue;
                            const bool relation_matters = names_matter && hosted == 1;
                            if (!relation_matters && &relation != &relations[0]) continue;
                            Story s;
                            s.length = length;
                            s.kind = kind;
                            s.count = kind == 1 || kind == 0 || kind == 4 ? count : 5;
                            s.hosted = hosted;
                            s.score_one = relation.one;
                            s.score_two = relation.two;
                            s.flagged = arm == 0 ? 1 : 0;
                            s.end_year = arm == 2 ? 2005 : 0;
                            add(s, "wording");
                        }
    // The same, for the wordings whose competition is named, with the remark on the result for every score pair.
    constexpr Scores pairs[] = {{7, 0}, {4, 0}, {4, 2}, {2, 1}, {3, 3}, {1, 1}, {0, 0}, {0, 6}, {0, 4}, {2, 5}, {1, 3}, {-1, 2}, {-3, -3}, {-2, 3}, {127, -128}, {-128, 127}, {5, 0}, {6, 0}, {1, 0}, {0, 3}, {0, 5}, {1, 6}, {2, 2}};
    for (std::int32_t kind : {1, 2, 3})
        for (const auto& pair : pairs) {
            Story s;
            s.length = 1;
            s.kind = kind;
            s.count = 1;
            s.hosted = 0;
            s.score_one = pair.one;
            s.score_two = pair.two;
            add(s, "named competition, remark");
            s.hosted = 1;
            add(s, "result in the text");
            s.length = 4;
            add(s, "result in the text, several matches");
        }
    // The clubs' story of a ban with a named club pair and no fourth body (bodies[3] == 0, the message's refuse flag set).
    for (std::int32_t refuse : {1, 0x40, 0x100, -1})
        for (std::uint32_t hosted : {0u, 1u})
            for (const auto& pair : pairs) {
                Story s;
                s.bodies[3] = 0;
                s.refuse_four = refuse;
                s.hosted = hosted;
                s.score_one = pair.one;
                s.score_two = pair.two;
                s.link = std::uint32_t(&pair - pairs) & 1;
                add(s, "clubs story");
            }
    for (std::int32_t refuse : {0, 1})
        for (std::int32_t body3 : {0, 5, -1, 0x7f, 0x80}) {
            Story s;
            s.bodies[3] = body3;
            s.refuse_four = refuse;
            add(s, "clubs story condition");
        }
    // No fixture: the main text names the clubs the fixture object holds from its initialisation.
    for (bool path_a : {false, true})
        for (std::int32_t length : {-2, 1, 3}) {
            Story s;
            s.has_fixture = false;
            s.length = length;
            s.kind = 2;
            if (path_a) { s.bodies[3] = 0; s.refuse_four = 1; }
            add(s, "no fixture");
        }
    // The extra sentence, the name link and the fixture link.
    for (std::uint32_t extra : {0u, 1u, 0x100u, 0xffu})
        for (std::uint32_t matched : {0u, 1u})
            for (std::uint32_t link : {0u, 1u})
                for (bool path_a : {false, true}) {
                    Story s;
                    s.extra = extra;
                    s.matched = matched;
                    s.link = link;
                    s.kind = 1;
                    s.length = 3;
                    if (path_a) { s.bodies[3] = 0; s.refuse_four = 1; }
                    add(s, "tail");
                }
    // A kind the story does not know, and extreme lengths and fields.
    for (std::int32_t kind : {5, 6, -1, 0x7f, 0x80, 0x100, 0x101})
        for (std::int32_t length : {-2, 1, 3}) {
            Story s;
            s.kind = kind;
            s.length = length;
            add(s, "unknown kind");
        }
    for (std::int32_t length : {-128, -1, 0x7f, 0x80, 0x100, 0x101, 0xffff})
        for (std::int32_t kind : {0, 1}) {
            Story s;
            s.length = length;
            s.kind = kind;
            s.date = 0x12345678;
            s.end_year = 0x10005;
            add(s, "extreme length");
        }
    for (std::uint32_t end_year : {1u, 0x8000u, 0xffffu, 0x10000u}) {
        Story s;
        s.end_year = end_year;
        s.end_day = 0x8001;
        add(s, "end date");
    }
    // The competition test: a cup competition with the kinds and groups that are never named.
    for (std::uint32_t cup : {0u, 1u})
        for (std::uint32_t fixture_kind : {0x418u, 0x419u, 0xa0u, 0x417u, 0u})
            for (std::uint32_t group : {0u, 1u})
                for (std::uint32_t hosted : {0u, 1u}) {
                    Story s;
                    s.length = 3;
                    s.kind = 2;
                    s.hosted = hosted;
                    s.cup = cup;
                    s.fixture_kind = fixture_kind;
                    s.fixture_group = group;
                    add(s, "competition test");
                }
    // The lookups running for real, over the real clubs and competitions.
    for (std::int32_t length : {-2, 1, 3})
        for (std::int32_t kind : {1, 2, 3}) {
            Story s;
            s.real_lookups = true;
            s.length = length;
            s.kind = kind;
            add(s, "real lookups");
            s.bodies[3] = 0;
            s.refuse_four = 1;
            add(s, "real lookups, clubs story");
        }
    for (unsigned competition : {0u, 1u, 3u, 9u, 30u})
        for (std::int32_t kind : {1, 2, 3}) {
            Story s;
            s.real_lookups = true;
            s.competition = competition;
            s.kind = kind;
            s.length = 1;
            add(s, "real lookups, competition");
        }
    // The caller's callee-saved registers at entry (r14, r17-r25 are the caller's until the routine loads them): the early
    // calls (the field reads, the decoders, the lookups) save them in stack bytes a link row copies.
    for (bool body : {true, false})
        for (bool real : {false, true}) {
            Story s;
            s.body = body;
            s.real_lookups = real;
            s.kind = 2;
            s.length = 3;
            auto item = make(scenario, s, "caller registers");
            item.label += std::to_string(n++);
            for (unsigned r : {13u, 14u, 17u, 18u, 19u, 20u, 21u, 22u, 23u, 24u, 25u}) item.registers.push_back({r, 0xc0de0000u + r});
            cases.push_back(std::move(item));
        }
    // The first named body in slot 1, 2 or 3 (byte 0 is -1), with the message flag clear and set, for the dated, one-match
    // and several-match texts.
    for (unsigned slot : {1u, 2u, 3u})
        for (std::int32_t flagged : {0, 1})
            for (std::int32_t length : {-2, 1, 3}) {
                Story s;
                for (auto& body : s.bodies) body = -1;
                s.bodies[slot] = 5 + std::int32_t(slot);
                s.flagged = flagged;
                s.length = length;
                s.kind = 2;
                add(s, "first body in slot");
            }
}
