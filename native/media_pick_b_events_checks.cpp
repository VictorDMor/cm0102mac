// Cases for the press's event pass (0x10698f50): event rows laid out in the scratch area over real
// persons and clubs, the callees answered, and the random table scripted so each draw is chosen.
#include "media_checks.hpp"

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t entry_events = 0x10698f50;
constexpr std::uint32_t press_at = scratch_base, found_at = scratch_base + 0x100, standing_at = scratch_base + 0x200,
    cache_at = scratch_base + 0x300, rows_at = scratch_base + 0x4000, table_at = scratch_base + 0x8000, stack_top = scratch_base + 0x80000;
// The callees.
constexpr std::uint32_t standing_of = 0x10082f20, record_of = 0x10082f90, cached = 0x103f83c0, club_flag = 0x1014c2e0,
    person_flag = 0x100d3150, fame = 0x100d71a0, listed_clubs = 0x103fc710, rival_report = 0x100e9d10;

// Distinctive values under the argument, so what the routine leaves of r3-r10 and f1 shows.
Registers entry_registers(std::uint32_t press_object) {
    return {{1, stack_top}, {3, press_object}, {4, 0x41414141}, {5, 0x51515151}, {6, 0x61616161}, {7, 0x71717171}, {8, 0x81818181},
            {9, 0x91919191}, {10, 0xa1a1a1a1}};
}
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(Writes& out, const Writes& more) { out.insert(out.end(), more.begin(), more.end()); }
Writes random_table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, bool refill = false) {
    Writes writes;
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back({2, table_at + 2 * std::uint32_t(i + 1), entries[i]});
    const auto end = table_at + 0x100;
    writes.push_back({4, s.toc(-0x7b84), refill ? end : table_at});
    writes.push_back({4, s.toc(-0x7b88), end});
    writes.push_back({4, s.toc(-0x7b8c), table_at});
    writes.push_back({4, s.toc(-0x7b90), seed});
    if (refill) for (std::uint32_t i = 1; i <= 4; ++i) writes.push_back({2, table_at + 2 * i, 0x1111u * i});
    return writes;
}

struct Row {
    std::uint32_t person, club;
    int day, year;
    unsigned mark = 0;
    std::uint32_t story = 7, weight = 100;
};
Writes event_rows(const std::vector<Row>& rows, const std::array<std::uint32_t, 3>& history) {
    Writes out = {w32(press_at, history[0]), w32(press_at + 4, history[1]), w32(press_at + 8, history[2]), w32(press_at + 0xc, rows_at), w32(press_at + 0x10, 20),
                  w32(press_at + 0x14, std::uint32_t(rows.size()))};
    for (std::size_t n = 0; n < rows.size(); ++n) {
        const auto at = rows_at + std::uint32_t(n) * 0x1c;
        const auto& r = rows[n];
        append(out, {w32(at, r.person), w32(at + 4, r.club), h16(at + 8, std::uint32_t(r.day)), h16(at + 0xa, std::uint32_t(r.year)), w32(at + 0xc, 0),
                     b8(at + 0x10, r.mark), w32(at + 0x14, r.story), w32(at + 0x18, r.weight)});
    }
    return out;
}
}

MEDIA_CASES("pick_b_events") {
    const auto persons = scenario.global(-0x7be0);
    const auto person_total = scenario.global(-0x7be4);
    const auto today_word = scenario.r32(scenario.toc(-0x7b80));
    const int today_day = std::int16_t(today_word >> 16), today_year = int(today_word & 0xffff);
    // The live press's history table, so the module's own routines (which search it) find a real one.
    const auto live_press = scenario.global(-0x7b94);
    const std::array<std::uint32_t, 3> history = {scenario.r32(live_press), scenario.r32(live_press + 4), scenario.r32(live_press + 8)};
    const auto person_at = [&](std::uint32_t index) { return persons + index * 0x70; };
    // Real people with a contract and the three links the story reads, and real clubs with a manager
    // who has them too.
    std::vector<std::uint32_t> people;
    for (std::uint32_t i = 0; people.size() < 8 && i < 20000; ++i) {
        const auto p = person_at(i);
        if (scenario.r32(p + 0x14) && scenario.r32(p + 0x28) && scenario.r32(p + 0x2c) && scenario.r32(p + 0x30) && scenario.r32(p + 0x24)) {
            const auto home = scenario.r32(p + 0x24), manager = scenario.r32(home + 0x90);
            if (manager && scenario.r32(manager + 0x28) && scenario.r32(manager + 0x2c) && scenario.r32(manager + 0x30)) people.push_back(i);
        }
    }
    std::vector<std::uint32_t> clubs;
    for (std::uint32_t i = 0; clubs.size() < 6 && i < 400; ++i) {
        const auto manager = scenario.r32(scenario.club(i) + 0x90);
        if (manager && scenario.r32(manager + 0x28) && scenario.r32(manager + 0x2c) && scenario.r32(manager + 0x30)) clubs.push_back(i);
    }
    const auto contract = [&](std::uint32_t index) { return scenario.r32(person_at(index) + 0x14); };
    // The clubs' index that wraps to a null club pointer, if the table is aligned for it.
    const auto base = scenario.global(-0x7be8);

    struct Setup {
        std::vector<Row> rows;
        Answers answers;
        Writes memory;
        std::vector<std::uint32_t> draws = {1, 1, 1, 1};  // no story unless a case draws one
        std::uint32_t seed = 0;
        std::uint32_t own_club;
    };
    const auto A = people[0], B = people[1];
    const auto X = clubs[0], Y = clubs[1], Z = clubs[2];
    const auto fresh = [&](std::uint32_t p, std::uint32_t k, unsigned mark = 0) { return Row{p, k, today_day, today_year, mark, 7, 100}; };
    const auto answers_default = [&](std::uint32_t own) -> Answers {
        (void)own;
        return {{standing_of, found_at}, {record_of, standing_at}, {cached, 1}, {club_flag, 0}, {person_flag, 0}, {fame, 5}, 
                {listed_clubs, 0}, {rival_report, 0}};
    };
    const auto set_answer = [](Answers& answers, std::uint32_t entry, std::uint32_t value) {
        for (auto& a : answers) if (a.first == entry) a.second = value;
    };
    const auto run = [&](const char* label, const Setup& s) {
        Writes memory = event_rows(s.rows, history);
        append(memory, random_table(scenario, s.draws, s.seed));
        append(memory, {w32(found_at + 4, s.own_club), b8(standing_at + 0x44, 60), b8(standing_at + 0x54, 0), w32(scenario.toc(-0x7b40), cache_at)});
        append(memory, s.memory);
        cases.push_back({.entry = entry_events, .registers = entry_registers(press_at), .memory = std::move(memory), .f1 = 1234.5, .label = label, .answers = s.answers});
    };
    auto base_setup = [&]() {
        Setup s;
        s.own_club = X;
        s.answers = answers_default(X);
        return s;
    };

    // The story cases run the module's own routines for real, and those need the game's own standing
    // records: the lookups are not answered there, and people are real members of their clubs.
    const auto story_setup = [&]() {
        auto s = base_setup();
        s.draws = {25, 100, 7, 3};
        std::erase_if(s.answers, [](const auto& a) { return a.first == standing_of || a.first == record_of; });
        return s;
    };

    // Nothing to do.
    { auto s = base_setup(); run("no rows", s); }

    // Old rows are dropped: by year, by day, at the boundary of 31 days; first, middle and last of several.
    {
        for (const int offset : {0, -1, -30, -31, -32, -40, -400}) {
            auto s = base_setup();
            const int day = today_day + offset;
            s.rows = {Row{A, X, day, today_year, 0, 7, 100}};
            run(("one row at day " + std::to_string(offset)).c_str(), s);
        }
        auto s = base_setup();
        s.rows = {Row{A, X, today_day, today_year - 1, 0, 7, 100}};
        run("one row last year", s);
        s.rows = {Row{A, X, today_day, today_year - 2, 0, 7, 100}};
        run("one row two years ago", s);
        s.rows = {Row{A, X, today_day, today_year + 1, 1, 7, 100}};
        run("one row next year", s);
        for (unsigned old_mask = 0; old_mask < 16; ++old_mask) {
            auto t = base_setup();
            for (unsigned n = 0; n < 4; ++n)
                t.rows.push_back(Row{people[n], clubs[n % 3], today_day, today_year - int((old_mask >> n) & 1), 1, std::uint32_t(10 + n), 50u + n});
            run("four rows, some old", t);
        }
        // No manager at the club, no contract, a null standing, the lookup failing.
        auto t = base_setup();
        t.rows = {fresh(A, X), fresh(B, Y)};
        auto no_manager = t;
        no_manager.memory = {w32(scenario.club(X) + 0x90, 0)};
        run("no manager", no_manager);
        auto no_contract = t;
        no_contract.memory = {w32(person_at(A) + 0x14, 0)};
        run("no contract", no_contract);
        auto no_standing = t;
        set_answer(no_standing.answers, standing_of, 0);
        run("no standing", no_standing);
        auto not_cached = t;
        set_answer(not_cached.answers, cached, 0);
        run("not in cache", not_cached);
        auto no_record = t;
        set_answer(no_record.answers, record_of, 0);
        run("no standing record", no_record);
        auto two_rows_story = t;
        run("two rows, cache", two_rows_story);
        auto one_row_no_cache = base_setup();
        one_row_no_cache.rows = {fresh(A, X)};
        one_row_no_cache.memory = {w32(scenario.toc(-0x7b40), 0)};
        run("one row no cache object", one_row_no_cache);
        auto no_cache_object = t;
        no_cache_object.memory = {w32(scenario.toc(-0x7b40), 0)};
        run("no cache object", no_cache_object);
    }

    // A marked row (mark 1) closes the chapter: the date against the person's, the standing record,
    // the draws (50 + 40 against the byte, one in 30), then every row of the person is marked 2.
    {
        const auto person = person_at(A);
        for (const int person_year : {today_year - 1, today_year, today_year + 1})
            for (const int person_day : {int(today_day) - 5, int(today_day), int(today_day) + 5, -3, 400})
                for (const int row_day : {int(today_day), int(today_day) - 3, -5, 300}) {
                    auto s = base_setup();
                    s.rows = {Row{A, X, row_day, today_year, 1, 7, 100}, Row{A, Y, today_day, today_year, 0, 8, 90}, Row{B, Z, today_day, today_year, 1, 9, 80}};
                    s.memory = {h16(person + 0x4c, std::uint32_t(person_day)), h16(person + 0x4e, std::uint32_t(person_year))};
                    s.draws = {17, 30, 60};  // x % 50 = 17 -> threshold 57 against the byte 60
                    run("marked row, dates", s);
                }
        for (const std::uint32_t mark : {0u, 1u, 2u, 3u}) {
            auto s = base_setup();
            s.rows = {fresh(A, X, mark)};
            s.memory = {h16(person + 0x4e, today_year - 1)};
            s.draws = {1, 30, 60};  // a draw that fails the one in 25 for fresh rows
            run("mark values", s);
        }
        for (const std::uint32_t byte44 : {0u, 57u, 58u, 59u, 100u, 127u, 128u, 200u, 255u})
            for (const auto& draws : std::vector<std::vector<std::uint32_t>>{{17, 30}, {17, 31}, {17, 0}, {67, 30}, {0, 60}, {49, 90}}) {
                auto s = base_setup();
                s.rows = {fresh(A, X, 1), fresh(A, Y, 0), fresh(B, Z, 1), fresh(A, Z, 1)};
                s.memory = {h16(person + 0x4e, today_year - 1), b8(standing_at + 0x44, byte44)};
                s.draws = draws;
                run("marked row, draws", s);
            }
        {
            auto s = base_setup();
            s.rows = {fresh(A, X, 1)};
            s.memory = {h16(person + 0x4e, today_year - 1)};
            s.draws = {17, 30};
            for (const std::uint32_t flag : {1u, 0u}) {
                auto t = s;
                set_answer(t.answers, club_flag, flag);
                run("marked row, barred club", t);
                t = s;
                set_answer(t.answers, person_flag, flag);
                run("marked row, flagged person", t);
            }
            auto t = s;
            t.memory.push_back(b8(standing_at + 0x54, 1));
            run("marked row, standing +0x54", t);
            t = s;
            set_answer(t.answers, record_of, 0);
            run("marked row, no standing record", t);
            t = s;
            t.draws = {17, 30};
            t.memory.push_back(h16(person + 0x4e, today_year + 3));
            run("marked row, later person", t);
        }
    }

    // A fresh row drawn (one in 25): the story. Youth draws and the float test of the weight, then the
    // fields, the clubs by rating, the audience and the follow-ups.
    {
        const auto person = person_at(A);
        for (const auto [b65, b67] : {std::pair<std::uint32_t, std::uint32_t>{5, 5}, {20, 5}, {5, 20}, {14, 14}, {15, 14}, {0x80, 3}, {3, 0xf0}})
            for (const std::uint32_t half : {0x7fffu, 0xfffbu, 0u, 40u})
                for (const std::uint32_t second : {100u, 30000u, 0xffffu, 0x4000u})
                    for (const std::uint32_t weight : {0u, 5000u, 100000000u, 0xfffffffbu, 0x7fffffffu, 0x80000000u, 70u}) {
                        auto s = story_setup();
                        s.rows = {Row{A, X, today_day, today_year, 0, 11, weight}};
                        s.memory = {b8(person + 0x65, b65), b8(person + 0x67, b67), h16(contract(A) + 0xc, half)};
                        s.draws = {25, second, 7};
                        run("story, youth draw", s);
                    }
        for (const std::uint32_t weight : {499u, 500u, 501u})
            for (const std::uint32_t skill : {0u, 2u}) {
                auto s = story_setup();
                s.rows = {Row{A, X, today_day, today_year, 0, 11, weight + skill * 25 / 2}};
                s.memory = {b8(person + 0x65, skill), b8(person + 0x67, 5), h16(contract(A) + 0xc, 0x7fff)};
                s.draws = {25, 100, 7};
                run("story, float boundary", s);
            }
        for (const std::uint32_t second : {0u, 1u, 2u, 0xfffeu, 0xffffu}) {
            auto s = story_setup();
            s.rows = {fresh(A, X)};
            s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
            s.draws = {50, second, 7};
            s.seed = second & 1 ? 0x80000001u : 0;
            run("story, plain draw", s);
        }
        // The first draw wrapping the table.
        {
            auto s = story_setup();
            s.rows = {fresh(A, X)};
            s.draws = {25, 100};
            Writes memory;
            append(memory, random_table(scenario, {25, 100, 7}, 0, true));
            s.memory = memory;
            run("story, table wraps", s);
        }
        // Several rows of one person: their clubs gather, five at most; a taken row cancels.
        for (const unsigned extra : {0u, 1u, 2u, 4u, 5u, 7u}) {
            auto s = story_setup();
            s.rows = {fresh(A, X)};
            for (unsigned n = 0; n < extra; ++n) s.rows.push_back(fresh(n % 2 ? A : B, clubs[(n + 1) % clubs.size()]));
            s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
            run("story, several rows", s);
        }
        for (const unsigned taken_at : {1u, 2u, 3u}) {
            auto s = story_setup();
            s.rows = {fresh(A, X), fresh(A, Y), fresh(A, Z), fresh(A, X)};
            s.rows[taken_at].mark = 1;
            s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
            run("story, taken row", s);
        }
        {
            auto s = story_setup();
            s.rows = {fresh(B, Y), fresh(A, X), fresh(A, Z), fresh(B, X), fresh(A, Y), fresh(A, X), fresh(A, Z), fresh(A, Y)};
            s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
            run("story, a row in the middle", s);
            s.rows[0].mark = 2;
            run("story, a row in the middle, other marked 2", s);
        }
        // Clubs ranked by rating.
        for (const auto& ratings : std::vector<std::vector<std::int32_t>>{{100, 200, 300}, {300, 200, 100}, {200, 200, 200}, {-100, 100, 0}, {5, -3000, 32767}, {-32768, 32767, 0}}) {
            auto s = story_setup();
            s.rows = {fresh(A, X), fresh(A, Y), fresh(A, Z)};
            s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
            const std::uint32_t order[3] = {X, Y, Z};
            for (unsigned n = 0; n < 3; ++n) s.memory.push_back(h16(scenario.club(order[n]) + 0x1ec, std::uint32_t(ratings[n])));
            run("story, ranking", s);
        }
        // The fields: club barred or not, the verdict values, the manager, the flags.
        for (const std::uint32_t barred : {0u, 1u, 0x100u})
            for (const std::uint32_t flag : {0u, 1u, 0x100u}) {
                auto s = story_setup();
                s.rows = {fresh(A, X), fresh(A, Y)};
                s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
                set_answer(s.answers, club_flag, barred);
                set_answer(s.answers, person_flag, flag);
                run("story, fields", s);
            }
        // The module's own routines (verdict, reaction, follow-up, comment) run for real here: answers would
        // bypass them on one side only when they are checked in the same run. Their results vary with the
        // person and club.
        for (const auto who : people)
            for (const auto at : clubs) {
                auto s = story_setup();
                s.rows = {fresh(who, at)};
                s.own_club = at;
                s.memory = {b8(person_at(who) + 0x65, 5), b8(person_at(who) + 0x67, 5), h16(contract(who) + 0xc, 0x7fff)};
                s.draws = {25, 100};
                run("story, reaction", s);
            }
        // A row whose club is the null pointer (an index wrapping the table) and an own club that is.
        {
            std::uint32_t inverse = 1;  // 73^-1 mod 2^32, by Newton's iteration
            for (int n = 0; n < 6; ++n) inverse *= 2 - 73 * inverse;
            if ((base & 7) == 0) {
                const std::uint32_t wrap = std::uint32_t(0u - base) / 8 * inverse;
                auto s = story_setup();
                s.rows = {fresh(A, wrap)};
                s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
                for (unsigned n = 0; n < 16; ++n) s.memory.push_back(b8(person_at(person_total - 1 - n) + 0x6b, 0));
                run("story, row club null", s);
                auto t = base_setup();
                t.rows = {fresh(A, X)};
                t.own_club = wrap;
                t.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
                for (unsigned n = 0; n < 16; ++n) t.memory.push_back(b8(person_at(person_total - 1 - n) + 0x6b, 0));
                run("story, own club null", t);
                t.rows = {fresh(A, wrap), fresh(B, X)};
                run("story, own club null, two", t);
            }
        }
        // The audience: the listed clubs, the rows' clubs, and the newest sixteen people.
        for (const std::uint32_t listed : {0u, 1u, 4u, 28u, 29u, 30u, 31u, 0xffu, 0x80u}) {
            auto s = story_setup();
            s.rows = {fresh(A, X), fresh(A, Y), fresh(A, Z)};
            s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
            set_answer(s.answers, listed_clubs, listed);
            run("story, audience size", s);
            s.memory.push_back(w32(scenario.toc(-0x7b40), 0));
            run("story, no cache", s);
        }
        {
            // The sixteen newest: every combination of the byte at +0x6b, the club link at +0x24 and the two ways to be wanted.
            const auto total = person_total;
            const auto foreign30 = scenario.r32(scenario.club(9) + 0x30), foreign2c = scenario.r32(scenario.club(9) + 0x2c);
            for (unsigned variant = 0; variant < 12; ++variant)
                for (const std::uint32_t listed : {0u, 5u, 27u, 29u}) {
                    auto s = story_setup();
                    s.rows = {fresh(A, X), fresh(A, Y)};
                    s.memory = {b8(person + 0x65, 30), b8(person + 0x67, 30)};
                    set_answer(s.answers, listed_clubs, listed);
                    for (unsigned n = 0; n < 16; ++n) {
                        const auto p = person_at(total - 1 - n);
                        const auto k = scenario.club((n + variant) % 5);
                        const unsigned kind = (n + variant) % 6;
                        append(s.memory, {b8(p + 0x6b, kind == 0 ? 0 : 1 + n), w32(p + 0x24, kind == 1 ? 0 : k)});
                        // kinds 2-5: the person's club shares the +0x30 record, or the +0x2c record with a lower rating, or not.
                        const auto involved = scenario.club(n % 2 ? Y : X);
                        if (kind == 2) append(s.memory, {w32(k + 0x30, scenario.r32(involved + 0x30))});
                        if (kind == 3) append(s.memory, {w32(k + 0x30, foreign30), w32(k + 0x2c, scenario.r32(involved + 0x2c)), h16(k + 0x1ec, 0x100), h16(involved + 0x1ec, 0x1200)});
                        if (kind == 4) append(s.memory, {w32(k + 0x30, foreign30), w32(k + 0x2c, scenario.r32(involved + 0x2c)), h16(k + 0x1ec, 0x1100), h16(involved + 0x1ec, 0x1200)});
                        if (kind == 5) append(s.memory, {w32(k + 0x30, foreign30), w32(k + 0x2c, foreign2c)});
                    }
                    run("story, newest people", s);
                }
        }
        // After the story: the club's flag, the verdict (zero and not), the follow-ups.
        for (const std::uint32_t barred : {0u, 1u})
            for (const auto who : people) {
                auto s = story_setup();
                s.rows = {fresh(who, X), fresh(who, Y)};
                s.memory = {b8(person_at(who) + 0x65, 5), b8(person_at(who) + 0x67, 5), h16(contract(who) + 0xc, 0x7fff)};
                s.draws = {25, 100};
                set_answer(s.answers, club_flag, barred);
                run("story, after", s);
            }
    }

    // The real events and the real callees: the persons' own standings and clubs.
    {
        for (const auto& draws : std::vector<std::vector<std::uint32_t>>{{25, 100, 7, 3}, {1, 2, 3}, {50, 100, 17, 30}, {0, 0, 0, 0, 0, 0}}) {
            Setup s;
            s.own_club = X;
            for (unsigned n = 0; n < 8; ++n) s.rows.push_back(Row{people[n % people.size()], clubs[n % clubs.size()], today_day, today_year, n % 3 == 0 ? 1u : 0u, 5, 50});
            s.draws = draws;
            Writes memory = event_rows(s.rows, history);
            append(memory, random_table(scenario, s.draws, 0));
            cases.push_back({.entry = entry_events, .registers = entry_registers(press_at), .memory = memory, .f1 = 1234.5, .label = "real callees", .answers = {}});
        }
        // Real persons of every kind (with and without contracts, clubs or managers), real standings.
        for (const std::uint32_t start : {0u, 3000u, 20000u, 50000u, 90000u})
            for (const auto& draws : std::vector<std::vector<std::uint32_t>>{{25, 100, 7, 3, 50, 100, 25, 7, 3, 0, 0, 0, 0}, {1, 2, 3}, {17, 30, 17, 30, 17, 30}}) {
                std::vector<Row> rows;
                for (unsigned n = 0; n < 12; ++n)
                    rows.push_back(Row{start + n * 7, clubs[n % clubs.size()], today_day, today_year, n % 4 == 1 ? 1u : 0u, 5 + n, 50 + n});
                Writes memory = event_rows(rows, history);
                append(memory, random_table(scenario, draws, 0));
                cases.push_back({.entry = entry_events, .registers = entry_registers(press_at), .memory = memory, .f1 = 1234.5, .label = "real callees, many persons", .answers = {}});
            }
        // Every fresh row draws its story, whatever the real lookups answer.
        for (const std::uint32_t start : {0u, 3000u, 20000u, 50000u, 90000u, 100000u})
            for (const std::uint32_t zero_mark : {0u, 1u}) {
                std::vector<Row> rows;
                for (unsigned n = 0; n < 12; ++n)
                    rows.push_back(Row{start + n * 7, clubs[n % clubs.size()], today_day, today_year, zero_mark && n % 3 == 2 ? 1u : 0u, 5 + n, 50 + n});
                Writes memory = event_rows(rows, history);
                append(memory, random_table(scenario, std::vector<std::uint32_t>(60, 0), 0));
                cases.push_back({.entry = entry_events, .registers = entry_registers(press_at), .memory = memory, .f1 = 1234.5, .label = "real callees, every draw", .answers = {{cached, 1}}});
            }
        // A person the real lookup finds no club for, given a contract and the links of a real person, after
        // one that has a club: the manager of the first row is still held (register r19 keeps it).
        {
            std::vector<std::uint32_t> clubless;
            for (std::uint32_t i = 0; clubless.size() < 6 && i < 100000; ++i)
                if (!scenario.r32(person_at(i) + 0x14)) clubless.push_back(i);
            for (const auto lone : clubless)
                for (const std::uint32_t second_mark : {0u, 1u}) {
                    const auto p = person_at(lone);
                    Writes memory = event_rows(std::vector<Row>{Row{A, X, today_day, today_year, 0, 7, 100}, Row{lone, Y, today_day, today_year, second_mark, 8, 90}}, history);
                    append(memory, random_table(scenario, std::vector<std::uint32_t>(60, 0), 0));
                    append(memory, {w32(p + 0x14, contract(A)), w32(p + 0x28, scenario.r32(person_at(A) + 0x28)), w32(p + 0x2c, scenario.r32(person_at(A) + 0x2c)),
                                    w32(p + 0x30, scenario.r32(person_at(A) + 0x30)), b8(p + 0x65, 30), b8(p + 0x67, 30), h16(p + 0x4e, today_year - 1)});
                    cases.push_back({.entry = entry_events, .registers = entry_registers(press_at), .memory = memory, .f1 = 1234.5, .label = "no club after a club", .answers = {{cached, 1}}});
                }
        }
        // The live press object.
        const auto live = scenario.global(-0x7b94);
        cases.push_back({.entry = entry_events, .registers = entry_registers(live), .memory = {}, .f1 = 1234.5, .label = "the live press", .answers = {}});
    }
}

// ---- Closing the last uncovered outcomes (central coverage run): the audience list (0x103fc710 answered, its array laid out in the
// frame) already holds the club of a row at a place before its end, so the duplicate scan stops on the match.
MEDIA_CASES("pick_b_events_closure") {
    const auto persons = scenario.global(-0x7be0);
    const auto today_word = scenario.r32(scenario.toc(-0x7b80));
    const int today_day = std::int16_t(today_word >> 16), today_year = int(today_word & 0xffff);
    const auto live_press = scenario.global(-0x7b94);
    const std::array<std::uint32_t, 3> history = {scenario.r32(live_press), scenario.r32(live_press + 4), scenario.r32(live_press + 8)};
    std::vector<std::uint32_t> people;
    for (std::uint32_t i = 0; people.size() < 2 && i < 20000; ++i) {
        const auto p = persons + i * 0x70;
        if (scenario.r32(p + 0x14) && scenario.r32(p + 0x28) && scenario.r32(p + 0x2c) && scenario.r32(p + 0x30) && scenario.r32(p + 0x24)) {
            const auto home = scenario.r32(p + 0x24), manager = scenario.r32(home + 0x90);
            if (manager && scenario.r32(manager + 0x28) && scenario.r32(manager + 0x2c) && scenario.r32(manager + 0x30)) people.push_back(i);
        }
    }
    std::vector<std::uint32_t> clubs;
    for (std::uint32_t i = 0; clubs.size() < 3 && i < 400; ++i) {
        const auto manager = scenario.r32(scenario.club(i) + 0x90);
        if (manager && scenario.r32(manager + 0x28) && scenario.r32(manager + 0x2c) && scenario.r32(manager + 0x30)) clubs.push_back(i);
    }
    if (people.empty() || clubs.size() < 3) return;
    const auto A = people[0];
    const auto X = clubs[0], Y = clubs[1], Z = clubs[2];
    const auto person = persons + A * 0x70;
    const auto contract = scenario.r32(person + 0x14);
    const auto audience = stack_top - 0x220 + 0x64;  // the frame's array of places (0x103fc710 fills it for real)

    const auto run = [&](const std::vector<std::uint32_t>& listed, std::uint32_t listed_count, const char* label) {
        std::vector<Row> rows;
        for (const auto club : {X, Y}) rows.push_back(Row{A, club, today_day, today_year, 0, 11, 100});
        Writes memory = event_rows(rows, history);
        append(memory, random_table(scenario, {25, 100, 7, 3}, 0));
        append(memory, {w32(found_at + 4, X), b8(standing_at + 0x44, 60), b8(standing_at + 0x54, 0), w32(scenario.toc(-0x7b40), cache_at),
                        b8(person + 0x65, 5), b8(person + 0x67, 5), h16(contract + 0xc, 0x7fff)});
        for (std::size_t n = 0; n < listed.size(); ++n) memory.push_back(w32(audience + 4 * std::uint32_t(n), scenario.club(listed[n])));
        // The standing lookups are the game's own (real people are members of their clubs); everything else outside the module is answered.
        cases.push_back({.entry = entry_events, .registers = entry_registers(press_at), .memory = std::move(memory), .f1 = 1234.5, .label = label,
                         .answers = {{cached, 1}, {club_flag, 0}, {person_flag, 0}, {fame, 5}, {listed_clubs, listed_count}, {rival_report, 0}}});
    };
    // The row clubs X then Y: X listed first (found with the list longer than one), Y listed last, both, neither, the list longer.
    run({X, Y}, 2, "audience holds the first row club");
    run({Y, X}, 2, "audience holds the first row club last");
    run({X, Z, Y}, 3, "audience holds both row clubs");
    run({Z, Y, X, Z}, 4, "audience holds both, later");
    run({Z, X}, 2, "audience holds the club last");
    run({Z, Z}, 2, "audience holds neither");
    run({X}, 1, "audience holds one");
}
