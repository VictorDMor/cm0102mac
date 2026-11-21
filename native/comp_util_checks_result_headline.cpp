// COMP_UTIL.CPP, block `result_headline`: cases for the news item about a club's result in a fixture (0x10464d00).
//
// The routine reads a fixture row (the game's 0x50-byte match row: ids, two club pointers, the competition record, a
// first leg, two round codes, two tie-break bytes and three pairs of scores), one club of the real table and a few
// records, and makes 20 kinds of call; the sentence it picks is one of 173 by a decision tree (see
// comp_util_headline.cpp). The cases build the rows in the scratch area (every offset here is 0xa0000 or above) from real
// values: the competition records and the club of the real state, the ids and the division cells the two special
// competitions are tested against. A case writes every field it uses, so the rows are reused from case to case.
//
// What the families cover (a case is the original against the rewrite, both with every callee traced: the format
// call's return address, the registers the formatter reads and the stack words it reads from sp+0x38 are compared, and
// so are the news item the appenders fill and the heap):
//   grid      random draws over every axis the tree tests: the result class (a round above 0, -2, other), the tie-break
//             bytes (every decider, negative values, equal and unequal pairs), home or away, the grade, who goes through,
//             a first leg (none, with a club record, without), the three score pairs, the first round code (the final: 150
//             or a code that rounds to it, with and without a second code), the language and the callee answers;
//   special   the two special competitions' ids with every league/division state of the two clubs (none, no division,
//             inside, outside), which selects the special record's name and gender;
//   leg       the first leg's name under every language group;
//   tail      the statistics, friendly and round-text answers (both ways) under the same sentence;
//   raw       register garbage above the byte and halfword each parameter uses;
//   norecord  a fixture without a competition record (the sentences that do not read its name gender; the friendly test answered).
// The news item is a scratch pointer: the appenders and the other callees really run. A fixture row whose competition
// record is null is not built (the original reads the record's name gender at address 0x6a). Labels start with
// "result_headline:".
#include "comp_util_checks.hpp"
#include <array>
#include <random>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t headline = 0x10464d00;
constexpr std::uint32_t format = 0x10727890, copy_entry = 0x1077dbf0, sprintf_entry = 0x1077b720, length = 0x1077dbd0,
                        begin_item = 0x1074b830, append_item = 0x1074bab0, stats = 0x1029eb20, date = 0x107156b0, friendly = 0x1048fa50;
constexpr std::uint32_t language_byte = 0x108bba90u - 0x35ac;  // the localization lookup's current language
constexpr std::uint32_t fixture_at = scratch_base + 0xa0000, first_at = scratch_base + 0xb0000, second_at = scratch_base + 0xb1000,
                        league_at = scratch_base + 0xb2000, division_at = scratch_base + 0xb3000, leg_at = scratch_base + 0xc0000,
                        leg_club_at = scratch_base + 0xc1000, item_at = scratch_base + 0xe0000;
constexpr std::uint32_t club_name = 0x221, club_gender = 0x23b, club_league = 0x2c, league_division = 0x74, division_id = 0x08;
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
std::string number(std::int64_t value) { return std::to_string(value); }
void put_text(std::vector<Write>& memory, std::uint32_t at, const std::string& text) {
    for (std::size_t i = 0; i <= text.size(); ++i) memory.push_back(w8(at + std::uint32_t(i), i < text.size() ? std::uint8_t(text[i]) : 0));
}
// An empty poke on each callee: the checker traces every case that has one.
std::vector<Case::Poke> traced() {
    std::vector<Case::Poke> pokes;
    // (not the language lookup 0x10727880: 0x104769d0's own call of it is the dispatcher's on the rewrite side only)
    for (const auto entry : {format, copy_entry, sprintf_entry, length, begin_item, append_item, stats, date, friendly}) pokes.push_back({entry, {}});
    return pokes;
}

// What one case builds.
struct Tie {
    bool home = true;                 // the club is the fixture's first club
    std::uint32_t club = 1;           // the real club (an index)
    std::uint32_t tiebreak_first = 0, tiebreak_second = 0;  // bytes +0x3d, +0x3e
    std::array<std::array<std::uint32_t, 2>, 3> scores = {{{0xff, 0xff}, {0xff, 0xff}, {1, 0}}};  // +0x4b/+0x4c, +0x47/+0x48, +0x43/+0x44 (-1: none)
    std::uint32_t code_30 = 160, code_32 = 0;   // the two round codes
    unsigned first_leg = 0;           // 0 none, 1 with a club record, 2 without
    unsigned special = 0;             // the fixture's competition id: 0 plain, 1 and 2 the two special competitions'
    std::array<unsigned, 2> chains = {0, 0};  // each club's league state: 0 none, 1 league without a division, 2 inside, 3 outside
    unsigned record_index = 1;        // the competition's record (an index of the real table; ~0: none)
    std::array<std::uint32_t, 2> genders = {0, 1};   // the two scratch clubs' name genders (bytes)
    std::uint32_t leg_gender = 1;
    std::uint32_t stats_leg = 0xffffffffu;   // the word the statistics look the fixture up by (-1: none)
    std::uint32_t date_a = 3, date_b = 2001;
    std::int32_t language = -1;       // -1 the snapshot's own
};

// The row, the clubs, the chains and the first leg of a tie; appended to `memory`.
std::uint32_t build(const Scenario& scenario, const Tie& tie, std::vector<Write>& memory) {
    const std::uint32_t special_ids[3] = {scenario.r32(scenario.toc(-0x6750)), scenario.r32(scenario.toc(-0x7ad8)), scenario.r32(scenario.toc(-0x7ae0))};
    const std::uint32_t expected[3] = {0, scenario.r32(scenario.toc(-0x7970)), scenario.r32(scenario.toc(-0x6b94))};
    const std::uint32_t plain_id = 0x12345;
    memory.push_back(w32(fixture_at + 0x00, tie.special == 0 ? plain_id : special_ids[tie.special]));
    memory.push_back(w32(fixture_at + 0x08, tie.stats_leg));
    memory.push_back(w32(fixture_at + 0x0c, tie.home ? tie.club : tie.club + 1));
    memory.push_back(w32(fixture_at + 0x10, tie.home ? tie.club + 1 : tie.club));
    memory.push_back(w32(fixture_at + 0x14, tie.record_index == ~0u ? 0 : scenario.competition(tie.record_index)));
    memory.push_back(w32(fixture_at + 0x18, tie.first_leg ? leg_at : 0));
    memory.push_back(w32(fixture_at + 0x1c, first_at));
    memory.push_back(w32(fixture_at + 0x20, second_at));
    memory.push_back(w16(fixture_at + 0x28, tie.date_a));
    memory.push_back(w16(fixture_at + 0x2a, tie.date_b));
    memory.push_back(w16(fixture_at + 0x30, tie.code_30));
    memory.push_back(w16(fixture_at + 0x32, tie.code_32));
    memory.push_back(w8(fixture_at + 0x3d, tie.tiebreak_first));
    memory.push_back(w8(fixture_at + 0x3e, tie.tiebreak_second));
    for (unsigned n = 0; n < 3; ++n) {
        constexpr std::uint32_t first[3] = {0x4b, 0x47, 0x43};
        memory.push_back(w8(fixture_at + first[n], tie.scores[n][0]));
        memory.push_back(w8(fixture_at + first[n] + 1, tie.scores[n][1]));
    }
    for (unsigned side = 0; side < 2; ++side) {
        const auto row = side == 0 ? first_at : second_at;
        put_text(memory, row + club_name, side == 0 ? "Ajax" : "Benfica");
        memory.push_back(w8(row + club_gender, tie.genders[side]));
        // The league chain: club +0x2c -> league +0x74 -> division +8 (an id the special competitions' cells hold or not).
        const auto league = league_at + side * 0x200, division = division_at + side * 0x200;
        const auto state = tie.chains[side];
        memory.push_back(w32(row + club_league, state == 0 ? 0 : league));
        memory.push_back(w32(league + league_division, state <= 1 ? 0 : division));
        memory.push_back(w32(division + division_id, tie.special == 0 ? 7 : state == 2 ? expected[tie.special] : expected[tie.special] + 1));
    }
    if (tie.first_leg) {
        put_text(memory, leg_at + 0x18, "Replay");
        memory.push_back(w32(leg_at + 0x10, tie.first_leg == 1 ? leg_club_at : 0));
        memory.push_back(w8(leg_at + 0x4b, tie.leg_gender));
        put_text(memory, leg_club_at + 0x1c, "Gamma");
    }
    if (tie.language >= 0) memory.push_back(w8(language_byte, std::uint32_t(tie.language)));
    return fixture_at;
}
std::string describe(const Tie& tie, std::uint32_t grade, std::uint32_t side, std::uint32_t round) {
    return "result_headline: " + std::string(tie.home ? "home" : "away") + " bytes " + number(tie.tiebreak_first) + "," + number(tie.tiebreak_second) + " grade " +
           number(grade) + " side " + number(side) + " round " + number(std::int32_t(round)) + " leg " + number(tie.first_leg) + " special " + number(tie.special) +
           " chains " + number(tie.chains[0]) + number(tie.chains[1]) + " codes " + number(tie.code_30) + "," + number(tie.code_32) + " scores " +
           number(tie.scores[0][0]) + "," + number(tie.scores[0][1]) + "/" + number(tie.scores[1][0]) + "/" + number(tie.scores[2][0]) + " language " + number(tie.language);
}
Case make(const Scenario& scenario, const Tie& tie, std::uint32_t grade, std::uint32_t side, std::uint32_t round, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {},
          const std::string& family = "") {
    std::vector<Write> memory;
    const auto row = build(scenario, tie, memory);
    return {.entry = headline, .registers = {{3, tie.club}, {4, row}, {5, grade}, {6, side}, {7, round}, {8, item_at}}, .memory = memory,
            .label = (family.empty() ? "" : family + " ") + describe(tie, grade, side, round), .answers = std::move(answers), .pokes = traced()};
}
// A small deterministic generator: the draws of a family do not depend on the standard library.
struct Draw {
    std::uint64_t state;
    std::uint32_t next(std::uint32_t bound) {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return std::uint32_t((state >> 33) % bound);
    }
    template <typename T, std::size_t N> T pick(const T (&values)[N]) { return values[next(N)]; }
};
const std::uint32_t bytes[] = {0, 0, 1, 3, 4, 5, 6, 7, 8, 9, 0xff, 0x80, 0x103};   // a tie-break byte (0x103: the byte 3 under garbage)
const std::uint32_t grades[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0xff, 0x102, 0xffffff05u};
const std::uint32_t sides[] = {0, 1, 2, 3, 4, 5, 0xff, 0x102, 0x1ff, 0xffffff04u};
const std::uint32_t rounds[] = {1, 5, 44, 100, 160, 374, 1000, 1155, 0x10005, 0xfffe, 0xfffffffeu, 0x1fffe, 0, 0xffff, 0x8000, 0x10000, 0xffff0003u};
const std::uint32_t codes_30[] = {0, 5, 150, 155, 149, 160, 1005, 999, 995, 0xfffb, 0xfff1, 0x7fff, 0x8000, 150, 150};
const std::uint32_t codes_32[] = {0, 0, 0, 0, 5, 0x453, 0x10000};
const std::uint32_t score_values[] = {0xff, 0xff, 0, 1, 2, 5, 0xfe, 0x80};
const unsigned record_indices[] = {0, 1, 7, 120};
const std::uint32_t dates_a[] = {0, 3, 100, 364}, dates_b[] = {2000, 2001, 2002, 2003};
const std::int32_t languages[] = {-1, -1, -1, 0, 1, 2, 3, 8};
}

COMP_UTIL_CASES("result_headline") {
    // The grid: random draws over every axis, the callees answering as they really do.
    Draw draw{0x9e3779b97f4a7c15ull};
    for (unsigned n = 0; n < 1600; ++n) {
        Tie tie;
        tie.home = draw.next(2) == 0;
        tie.club = draw.next(4);
        tie.tiebreak_first = draw.pick(bytes);
        tie.tiebreak_second = draw.pick(bytes);
        for (auto& pair : tie.scores) { pair[0] = draw.pick(score_values); pair[1] = draw.pick(score_values); }
        tie.code_30 = draw.pick(codes_30);
        tie.code_32 = draw.pick(codes_32);
        tie.first_leg = draw.next(3);
        tie.record_index = draw.pick(record_indices);
        tie.genders = {draw.next(4), draw.next(4)};
        tie.leg_gender = draw.next(4);
        tie.language = draw.pick(languages);
        tie.date_a = draw.pick(dates_a);
        tie.date_b = draw.pick(dates_b);
        const auto grade = draw.pick(grades), side = draw.pick(sides), round = draw.next(3) == 0 ? draw.pick(rounds) : draw.next(2) ? 0xfffeu : 160u;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
        if (draw.next(2)) answers.push_back({stats, draw.next(2)});
        if (draw.next(2)) answers.push_back({friendly, draw.next(2)});
        cases.push_back(make(scenario, tie, grade, side, round, answers, "grid"));
    }
}

COMP_UTIL_CASES("result_headline") {
    // The tree: the axes the decision tree tests, as a product (13824 combinations) of which an evenly spread eighth
    // is run (a fixed stride coprime to the product, so no axis aliases with it): the result class, the two tie-break
    // bytes (every decider and the negative bytes, with the other byte below, equal and above), home or away, the grade,
    // who goes through, a first leg, whether a deciding score exists, and the round codes that make a final.
    constexpr std::uint32_t decider_bytes[] = {0, 3, 4, 5, 6, 7, 8, 0xff}, other_bytes[] = {0, 9, 0xff};
    constexpr std::uint32_t tree_grades[] = {0, 2, 3, 5, 6, 7}, tree_rounds[] = {160, 0xfffe, 0};
    constexpr std::uint32_t final_codes[2][2] = {{160, 0}, {155, 0}};
    constexpr std::uint32_t total = 8 * 3 * 2 * 6 * 2 * 2 * 2 * 2 * 3;
    for (std::uint32_t n = 0; n < total / 4; ++n) {
        auto index = (n * 7919u) % total;
        const auto take = [&](std::uint32_t radix) { const auto value = index % radix; index /= radix; return value; };
        Tie tie;
        tie.tiebreak_first = decider_bytes[take(8)];
        tie.tiebreak_second = other_bytes[take(3)];
        tie.home = take(2) == 0;
        const auto grade = tree_grades[take(6)];
        const std::uint32_t side = take(2) ? 1 : 2;
        tie.first_leg = take(2);
        if (take(2) == 0) tie.scores[0] = {0xff, 0xff}; else tie.scores[0] = {2, 1};
        const auto& code = final_codes[take(2)];
        tie.code_30 = code[0];
        tie.code_32 = code[1];
        cases.push_back(make(scenario, tie, grade, side, tree_rounds[take(3)], {}, "tree"));
    }
}

COMP_UTIL_CASES("result_headline") {
    // The ties without a decider and without a negative byte (both bytes 0, or an unlisted nonzero first byte): the
    // sentences of the plain grades, under half of the product of the other axes.
    constexpr std::uint32_t pairs[4][2] = {{0, 0}, {0, 1}, {1, 2}, {2, 1}};
    constexpr std::uint32_t plain_grades[] = {0, 2, 3, 5, 6, 7}, plain_rounds[] = {160, 0xfffe, 0};
    constexpr std::uint32_t total = 4 * 2 * 6 * 2 * 2 * 2 * 2 * 3;
    for (std::uint32_t n = 0; n < total / 2; ++n) {
        auto index = (n * 1009u) % total;
        const auto take = [&](std::uint32_t radix) { const auto value = index % radix; index /= radix; return value; };
        Tie tie;
        const auto& pair = pairs[take(4)];
        tie.tiebreak_first = pair[0];
        tie.tiebreak_second = pair[1];
        tie.home = take(2) == 0;
        const auto grade = plain_grades[take(6)];
        const std::uint32_t side = take(2) ? 1 : 2;
        tie.first_leg = take(2);
        if (take(2) == 0) tie.scores[0] = {0xff, 0xff}; else tie.scores[0] = {2, 1};
        tie.code_30 = take(2) ? 155 : 160;
        cases.push_back(make(scenario, tie, grade, side, plain_rounds[take(3)], {}, "plain"));
    }
}

COMP_UTIL_CASES("result_headline") {
    // The two special competitions with every league state of the two clubs, under the result classes and a few ties.
    Draw draw{0x1234567ull};
    for (const unsigned special : {1u, 2u})
        for (unsigned first = 0; first < 4; ++first)
            for (unsigned second = 0; second < 4; ++second)
                for (const std::uint32_t round : {160u, 0xfffeu, 0u})
                    for (const bool home : {true, false}) {
                        Tie tie;
                        tie.special = special;
                        tie.chains = {first, second};
                        tie.home = home;
                        tie.tiebreak_first = draw.pick(bytes);
                        tie.tiebreak_second = draw.pick(bytes);
                        tie.code_30 = draw.pick(codes_30);
                        tie.code_32 = draw.pick(codes_32);
                        tie.language = draw.pick(languages);
                        cases.push_back(make(scenario, tie, draw.pick(grades), draw.pick(sides), round, {}, "special"));
                    }
    // A plain competition whose clubs have the same league states: the special branch is not taken.
    for (unsigned first = 0; first < 4; ++first)
        for (unsigned second = 0; second < 4; ++second) {
            Tie tie;
            tie.chains = {first, second};
            cases.push_back(make(scenario, tie, 3, 1, 160, {}, "special"));
        }
}

COMP_UTIL_CASES("result_headline") {
    // The first leg's name (a record with a club or without) under every language group, with and without a round.
    for (const std::int32_t language : {-1, 0, 1, 2, 3, 4, 5, 8})
        for (const unsigned leg : {1u, 2u})
            for (const std::uint32_t round : {160u, 0xfffeu, 0u})
                for (const std::uint32_t side : {1u, 2u}) {
                    Tie tie;
                    tie.first_leg = leg;
                    tie.language = language;
                    tie.tiebreak_first = 4;
                    tie.code_30 = 160;
                    cases.push_back(make(scenario, tie, 3, side, round, {}, "leg"));
                }
}

COMP_UTIL_CASES("result_headline") {
    // The tail: every answer of the statistics and the friendly test (answered, and the real lookups), under a result
    // with a round's text (the length call then finds a text) and one without.
    for (const std::uint32_t stats_answer : {0u, 1u, 2u})
        for (const std::uint32_t friendly_answer : {0u, 1u, 2u})
            for (const std::uint32_t round : {160u, 0xfffeu}) {
                Tie tie;
                tie.tiebreak_first = 5;
                std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
                if (stats_answer < 2) answers.push_back({stats, stats_answer});
                if (friendly_answer < 2) answers.push_back({friendly, friendly_answer});
                cases.push_back(make(scenario, tie, 2, 1, round, answers, "tail"));
            }
    // The real statistics and friendly lookups on the real fixture ids.
    for (unsigned leg = 0; leg < 4; ++leg) {
        Tie tie;
        tie.stats_leg = leg;
        tie.club = leg;
        cases.push_back(make(scenario, tie, 3, 1, 160, {}, "tail"));
    }
}

COMP_UTIL_CASES("result_headline") {
    // Register garbage: the grade and the side are bytes, the round a halfword, the club a word; the tie-break bytes
    // negative and equal.
    Draw draw{0xdeadbeefull};
    for (const std::uint32_t grade : {0x103u, 0xffffff02u, 0x100u})
        for (const std::uint32_t side : {0x102u, 0x100u, 0xffffffffu, 0x104u})
            for (const std::uint32_t round : {0x10005u, 0xffff0000u | 0xfffeu, 0xabcd0000u | 160u}) {
                Tie tie;
                tie.tiebreak_first = draw.pick(bytes);
                tie.tiebreak_second = draw.pick(bytes);
                cases.push_back(make(scenario, tie, grade, side, round, {}, "raw"));
            }
}
COMP_UTIL_CASES("result_headline") {
    // No competition record: the name texts take their plain path and the sentences that name the record are not chosen
    // (a final would read the record's name gender at address 0x6a, which both sides fault on alike). The friendly test is
    // answered: it reads the record.
    for (const std::uint32_t round : {160u, 0xfffeu, 0u})
        for (const std::uint32_t grade : {0u, 2u, 3u, 5u, 6u, 7u})
            for (const std::uint32_t side : {1u, 2u})
                for (const unsigned leg : {0u, 1u}) {
                    Tie tie;
                    tie.record_index = ~0u;
                    tie.first_leg = leg;
                    tie.code_30 = 160;
                    tie.tiebreak_first = grade == 0 ? 4 : 0;
                    cases.push_back(make(scenario, tie, grade, side, round, {{friendly, 1}, {stats, 1}}, "norecord"));
                }
}
}
