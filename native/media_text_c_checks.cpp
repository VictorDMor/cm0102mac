// Cases for the press story (0x1067fed0): story records laid out in the scratch area over real clubs, staff,
// nations and competitions. The wording is chosen by many independent numbers, so the cases cover the
// kinds and tones systematically, then the numbers that choose inside each, then random draws.
#include "media_checks.hpp"
#include <array>
#include <stdexcept>
#include <string>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t fake_table = scratch_base + 0xa000;
constexpr std::uint32_t record_at = scratch_base + 0x100, output_at = scratch_base + 0x2000, manager_at = scratch_base + 0x9000;
constexpr std::uint32_t none = 0xffffffffu;

void put32(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({4, at, value}); }

// The text object's own routines answer on both sides (the object is not under test), and so does the
// lookup of whether the match has a report.
std::vector<std::pair<std::uint32_t, std::uint32_t>> answers(std::uint32_t known) {
    return {{0x1074b830, 0}, {0x1074b960, 0}, {0x1074bab0, 0}, {0x1029eb20, known}};
}

struct Staff { std::uint32_t index, first, middle, last; };
std::vector<Staff> named_staff(const Scenario& scenario, unsigned wanted) {
    std::vector<Staff> out;
    const auto table = scenario.global(-0x7be0);
    for (std::uint32_t index = 1; index < 4000 && out.size() < wanted; ++index) {
        const auto row = table + index * 0x70;
        try {
            const auto a = scenario.r32(row + 0x28), b = scenario.r32(row + 0x2c), c = scenario.r32(row + 0x30);
            if (!a || !b || !c) continue;
            out.push_back({index, scenario.r32(a + 0x20), scenario.r32(b + 0x20), scenario.r32(c + 0x20)});
        } catch (const std::exception&) { break; }
    }
    return out;
}

// Everything a story is chosen by.
struct Story {
    bool full = true, matched = false, known = false;
    std::uint32_t kind = 0x29, tone = 0, club = 7, closing = 0, flags = 0, amount = 3, competition = 1;
    std::uint32_t nation = none, nation_a = none, nation_b = none, suitor = none, previous_club = none, legend = none;
    std::uint32_t other = 1;           // index into the named staff (none: no other)
    bool other_is_subject = false;
    std::uint32_t transfer_agreed = 0, rumour = 0, outcome = 0, job = 3, outlet_kind = none, outlet_region = none, position = 10, second_division = 0;
    std::uint32_t manager = 0;          // 0 none; 1 no club, same nation; 2 no club, other nation; 3 club, same nation; 4 club, other nation
    std::uint32_t goals_one = 2, goals_two = 1, second_club = 11;
    bool named = true;
    std::uint32_t competition_index = 0;  // the competition the match belongs to
    bool fake_competitions = false;     // the competition table is replaced by four rows: the friendly stage, the reserve stage and two real ones
    bool no_date = false;               // the match has no day and year (a competition's own name is then not extended)
    bool day_eight = false;             // the day's kind (0x10727880) answers 8
    bool subject_fourth = false;        // the subject's index is a multiple of 4
    bool club_without_nation = false;   // the club has no nation record (and the user's manager has none either)
    bool legend_nation = false;         // the nation field names the legend's nation
};

struct Maker {
    const Scenario& scenario;
    std::vector<Staff> staff;
    std::uint32_t user_row;
    Staff fourth;
    std::uint32_t other_nation_of(std::uint32_t club) const {
        for (std::uint32_t other = 5; other < 400; ++other)
            if (other != club && scenario.r32(scenario.club(other) + 0x2c) != scenario.r32(scenario.club(club) + 0x2c) && scenario.r32(scenario.club(other) + 0x2c))
                return scenario.r32(scenario.club(other) + 0x2c);
        return 0;
    }
    Case make(const Story& s, const std::string& label) const {
        const auto& subject = s.subject_fourth ? fourth : staff[0];
        const auto& other = s.other_is_subject ? subject : staff[1 + (s.other % 2)];
        const auto& legend = staff[2];
        const auto tweak = [&](std::uint32_t id) { return s.named ? id : id + 1; };
        const auto legend_nation_id = scenario.r32(scenario.r32(scenario.global(-0x7be0) + legend.index * 0x70 + 0xc) + 0x68);
        std::vector<std::uint32_t> v{subject.index, tweak(subject.first), subject.middle, subject.last, s.club, s.kind, 0xdead,
                                     s.legend_nation ? legend_nation_id : s.nation, s.tone,
                                     s.other == none ? none : other.index, tweak(other.first), other.middle, other.last, s.closing, s.flags, s.amount,
                                     s.competition, s.matched ? 0x100u : 0u};
        // The flag word is read through a signed byte: 1 and 0xff are the values worth having.
        v[17] = s.matched ? 1 : 0;
        if (s.matched) v.insert(v.end(), {150, 2001, s.competition_index, s.no_date ? 0u : 1u, s.no_date ? 0u : 2u, 4, s.club, s.second_club, s.goals_one, s.goals_two, 5});
        v.insert(v.end(), {s.suitor, s.transfer_agreed, 12, 2002, s.rumour, s.legend == none ? none : legend.index, tweak(legend.first), legend.middle,
                           legend.last, s.nation_a, s.nation_b, s.outcome, s.job, s.previous_club, s.outlet_kind, s.outlet_region, s.position,
                           s.second_division});
        Writes memory;
        for (std::size_t n = 0; n < v.size(); ++n) put32(memory, record_at + 8 + 4 * std::uint32_t(n), v[n]);
        // The user's manager: his club (if any) and his nation decide whether the story is told at home.
        const auto club_nation = s.club_without_nation || s.club == none ? 0 : scenario.r32(scenario.club(s.club) + 0x2c);
        if (s.club_without_nation) put32(memory, scenario.club(s.club) + 0x2c, 0);
        const auto other_nation = other_nation_of(s.club);
        if (s.manager == 0) put32(memory, user_row + 4, 0);
        else if (s.manager == 5) put32(memory, user_row + 4, scenario.global(-0x7be0) + other.index * 0x70);  // the other staff member is the user's manager
        else {
            put32(memory, user_row + 4, manager_at);
            put32(memory, manager_at + 0xc, s.manager == 1 ? club_nation : other_nation);
            put32(memory, manager_at + 0x24, s.manager >= 3 ? scenario.club(s.club) : 0);
            if (s.manager == 4) put32(memory, manager_at + 0x24, scenario.club(s.second_club));
        }
        if (s.fake_competitions) {
            // Four rows: the real row 3 with the friendly and reserve stage names written over it, then rows 3 and 87 as they are.
            const auto real = scenario.competition(3);
            const char* names[2]{"Friendly", "Reserve"};
            for (unsigned row = 0; row < 4; ++row) {
                const auto at = fake_table + row * 0x6c;
                const auto source = row == 3 ? scenario.competition(87) : real;
                for (unsigned word = 0; word < 0x6c; word += 4) put32(memory, at + word, scenario.r32(source + word));
                if (row < 2)
                    for (const unsigned offset : {0x1au, 0x4du})
                        for (unsigned k = 0; k < 20; ++k) memory.push_back({1, at + offset + k, k < std::string(names[row]).size() ? std::uint32_t(names[row][k]) : 0u});
            }
            put32(memory, scenario.toc(-0x7bbc), fake_table);
        }
        auto answered = answers(s.known ? 1 : 0);
        // (0x10727880 returns a byte of the program's data, at the TOC minus 0x35ac.)
        if (s.day_eight) memory.push_back({1, 0x108bba90u - 0x35ac, 8});
        return {.entry = 0x1067fed0, .registers = {{3, 0}, {4, record_at}, {5, s.full ? 1u : 0u}, {6, output_at}}, .memory = std::move(memory), .f1 = {},
                .label = label, .answers = std::move(answered)};
    }
};

struct Random {
    std::uint64_t state;
    std::uint32_t next() {
        state += 0x9e3779b97f4a7c15ull;
        auto z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return std::uint32_t((z ^ (z >> 31)) >> 8);
    }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + next() % values.size()); }
    bool chance(unsigned in) { return next() % in == 0; }
};
}

MEDIA_CASES("text_c") {
    auto staff = named_staff(scenario, 4);
    if (staff.size() < 3) throw std::runtime_error("text_c cases: no named staff in the snapshot");
    const auto user_base = scenario.toc(-0x7bb0);
    const auto user_row = user_base + std::uint32_t(std::int32_t(std::int16_t(scenario.r32(user_base + 0x2c58) >> 16))) * 0x2c0;
    Staff fourth = staff[0];
    for (const auto& member : named_staff(scenario, 60))
        if (member.index % 4 == 0 && member.index != staff[1].index && member.index != staff[2].index) { fourth = member; break; }
    const Maker maker{scenario, staff, user_row, fourth};
    Random random{0x7e57c0de};
    // Clubs with a nation, and the nations' ids (the position in the nation table).
    std::vector<std::uint32_t> clubs;
    for (std::uint32_t index = 5; index < 400 && clubs.size() < 12; ++index)
        if (scenario.r32(scenario.club(index) + 0x2c)) clubs.push_back(index);
    const auto nation_id = [&](std::uint32_t club) { return scenario.r32(scenario.r32(scenario.club(club) + 0x2c) + 0x68); };
    // The friendly and reserve stages (found by the name text at +0x4d), and any other competition.
    std::uint32_t friendly = 1, reserve = 2, league = 3;
    for (std::uint32_t index = 1; index < 300; ++index) {
        try {
            std::string name;
            for (unsigned k = 0; k < 12; ++k) name += char(scenario.s.memory.read<1>(scenario.competition(index) + 0x4d + k));
            if (name.rfind("Friendly", 0) == 0) friendly = index;
            if (name.rfind("Reserve", 0) == 0) reserve = index;
        } catch (const std::exception&) { break; }
    }
    unsigned n = 0;
    const auto name_of = [&](const Story& s, const char* what) {
        return std::string(what) + " " + std::to_string(n++) + " full=" + std::to_string(s.full) + " kind=" + std::to_string(s.kind) + " tone=" + std::to_string(int(s.tone)) +
               " manager=" + std::to_string(s.manager) + " closing=" + std::to_string(s.closing) + (s.matched ? " match" : "");
    };
    const auto fill = [&](Story& s) {
        s.club = clubs[random.next() % clubs.size()];
        s.nation = random.pick<std::uint32_t>({none, nation_id(s.club), nation_id(clubs[0]), 3});
        s.nation_a = random.pick<std::uint32_t>({none, nation_id(s.club), nation_id(clubs[1]), 4});
        s.nation_b = random.pick<std::uint32_t>({none, nation_id(s.club), nation_id(clubs[2]), 5});
        s.suitor = random.pick<std::uint32_t>({none, clubs[3], clubs[4], clubs[5]});
        s.previous_club = random.pick<std::uint32_t>({none, clubs[6], clubs[7], 100000});
        s.legend = random.pick<std::uint32_t>({none, 2, 2});
        s.closing = random.next() % 6;
        s.flags = random.pick<std::uint32_t>({0, 0x1000000, 0x2c0040, 0x240040, 0xc0040, 0x40040, 0x102040, 0x200840, 0x840, 0x40, 0x2c0000, 0x80, 0x1000040});
        s.amount = random.pick<std::uint32_t>({0, 3, 4, 150});
        s.competition = random.pick<std::uint32_t>({none, 1, 2, 3});
        s.other = random.pick<std::uint32_t>({0, 1, none});
        s.other_is_subject = random.chance(8);
        s.transfer_agreed = random.chance(2);
        s.rumour = random.chance(2);
        s.outcome = random.next() % 6;
        s.job = random.next() % 5;
        s.outlet_kind = random.pick<std::uint32_t>({none, 0, 1, 2, 3, 4, 5, 6, 7});
        s.outlet_region = random.pick<std::uint32_t>({none, 3});
        s.position = random.pick<std::uint32_t>({0x10, 0x18, 0x20, 0x23, 0x25, 0x40});
        s.second_division = random.chance(2);
        s.manager = random.next() % 5;
        s.named = !random.chance(6);
        s.known = random.chance(2);
        s.goals_one = random.pick<std::uint32_t>({0, 1, 2, 3, 0xfe});
        s.goals_two = random.pick<std::uint32_t>({0, 1, 2, 3});
        s.competition_index = random.pick<std::uint32_t>({friendly, reserve, league});
        s.second_club = clubs[random.next() % clubs.size()];
    };
    // Every kind (and the ones around the table), every tone, the manager's situation, and the closing sentence.
    for (const bool full : {true, false})
        for (std::uint32_t kind = 0x27; kind <= 0x36; ++kind)
            for (const int tone : {-1, 0, 1, 2, 3, 4, 5})
                for (std::uint32_t manager = 0; manager < (full ? 6u : 2u); ++manager)
                    for (const std::uint32_t closing : full ? std::initializer_list<std::uint32_t>{0, 1, 2, 3, 4} : std::initializer_list<std::uint32_t>{0}) {
                        if (!full && tone != 0 && tone != 1 && tone != 3) continue;
                        Story s;
                        fill(s);
                        s.full = full; s.kind = kind; s.tone = std::uint32_t(tone); s.manager = manager; s.closing = closing;
                        s.matched = kind == 0x34;
                        cases.push_back(maker.make(s, name_of(s, "sweep")));
                    }
    // The match report: the competition, the score, the club that wants him and how it ends.
    for (const std::uint32_t tone : {0u, 1u, 2u, 3u, 4u})
        for (const std::uint32_t competition_index : {friendly, reserve, league})
            for (const auto goals : std::initializer_list<std::pair<std::uint32_t, std::uint32_t>>{{2, 1}, {1, 2}, {1, 1}, {0xfe, 1}})
                for (const std::uint32_t suitor : {none, clubs[3]})
                    for (const std::uint32_t agreed : {0u, 1u})
                        for (const std::uint32_t rumour : {0u, 1u})
                            for (const std::uint32_t outcome : {0u, 1u, 2u, 3u, 4u}) {
                                if (suitor == none && (agreed || rumour)) continue;
                                Story s;
                                fill(s);
                                s.kind = 0x34; s.tone = tone; s.matched = true; s.competition_index = competition_index; s.goals_one = goals.first;
                                s.goals_two = goals.second; s.suitor = suitor; s.transfer_agreed = agreed; s.rumour = rumour; s.outcome = outcome;
                                s.closing = random.next() % 5; s.manager = random.next() % 2;
                                cases.push_back(maker.make(s, name_of(s, "match")));
                            }
    // Number-driven branches inside single kinds.
    const std::uint32_t kind_flags[]{0, 0x1000000, 0x1000840, 0x1200840, 0x1000040, 0x2c0040, 0x240040, 0xc0040, 0x40040, 0x102040, 0x200840, 0x840, 0x40, 0x80};
    for (const std::uint32_t flags : kind_flags)
        for (const int tone : {-5, -4, -1, 0, 1, 2, 3, 4, 5, 8})
            for (const std::uint32_t manager : {0u, 1u, 2u})
                for (const bool fourth_subject : {false, true}) {
                    Story s;
                    fill(s);
                    s.kind = 0x2b; s.flags = flags; s.tone = std::uint32_t(tone); s.manager = manager; s.subject_fourth = fourth_subject;
                    cases.push_back(maker.make(s, name_of(s, "flags")));
                }
    // The legend's nation, the previous club, and the nations named in the story.
    for (const std::uint32_t kind : {0x29u, 0x2au, 0x30u, 0x31u, 0x33u, 0x2du, 0x2fu})
        for (const bool legend_nation : {false, true})
            for (const std::uint32_t previous : {none, clubs[6]})
                for (const std::uint32_t legend : {none, 2u})
                    for (const std::uint32_t tone : {0u, 1u, 2u, 3u, 5u})
                        for (const std::uint32_t manager : {0u, 1u}) {
                            Story s;
                            fill(s);
                            s.kind = kind; s.legend_nation = legend_nation; s.previous_club = previous; s.legend = legend; s.tone = tone; s.manager = manager;
                            s.nation_a = random.pick<std::uint32_t>({none, nation_id(s.club)});
                            s.nation_b = random.pick<std::uint32_t>({none, nation_id(s.club), 6});
                            cases.push_back(maker.make(s, name_of(s, "legend")));
                        }
    // The quote of a legend left out: the remainder of the id and tone, including a negative one.
    for (const int tone : {-9, -6, -5, -2, -1, 0, 1, 2, 3, 6, 7})
        for (const std::uint32_t manager : {0u, 1u}) {
            Story s;
            fill(s);
            s.kind = 0x2e; s.legend = none; s.tone = std::uint32_t(tone); s.manager = manager; s.subject_fourth = tone % 2 == 0;
            cases.push_back(maker.make(s, name_of(s, "remainder")));
        }
    // The club that wants him is dropped when the user's manager is from its nation, unless the subject's id is a multiple of 4.
    for (const std::uint32_t manager : {1u, 2u, 3u, 4u})
        for (const bool fourth_subject : {false, true})
            for (const std::uint32_t agreed : {0u, 1u})
                for (const std::uint32_t rumour : {0u, 1u})
                    for (const std::uint32_t suitor : {clubs[3], clubs[4]}) {
                        Story s;
                        fill(s);
                        s.kind = 0x34; s.matched = true; s.manager = manager; s.subject_fourth = fourth_subject; s.transfer_agreed = agreed; s.rumour = rumour;
                        s.suitor = suitor; s.tone = random.next() % 3; s.goals_one = 1; s.goals_two = 2;
                        cases.push_back(maker.make(s, name_of(s, "suitor")));
                    }
    // The match report with real and invented competitions, and without a match at all.
    for (const bool fake : {false, true})
        for (const std::uint32_t index : {0u, 1u, 2u, 3u})
            for (const std::uint32_t tone : {0u, 1u, 2u, 3u, none}) {
                Story s;
                fill(s);
                s.kind = 0x34; s.matched = true; s.fake_competitions = fake; s.no_date = fake; s.competition_index = index; s.competition = 2 + index % 2; s.tone = tone;
                s.goals_one = 3; s.goals_two = 1; s.outcome = index + 1;
                cases.push_back(maker.make(s, name_of(s, "competition")));
            }
    for (const std::uint32_t tone : {0u, 2u}) {
        Story s;
        fill(s);
        s.kind = 0x34; s.matched = false; s.tone = tone;
        cases.push_back(maker.make(s, name_of(s, "no match")));
    }
    // The closing sentence by league position.
    for (const std::uint32_t kind : {0x27u, 0x2bu, 0x35u})
        for (const std::uint32_t closing : {1u, 2u, 3u})
            for (const std::uint32_t division : {0u, 1u})
                for (const std::uint32_t position : {0x10u, 0x18u, 0x20u, 0x22u, 0x23u, 0x24u, 0x25u, 0x40u})
                    for (const std::uint32_t manager : {0u, 5u}) {
                        Story s;
                        fill(s);
                        s.kind = kind; s.closing = closing; s.second_division = division; s.position = position; s.manager = manager;
                        s.tone = 4;
                        cases.push_back(maker.make(s, name_of(s, "position")));
                    }
    // The short story's remainders, including negative ones.
    for (const std::uint32_t kind : {0x29u, 0x2au, 0x2bu})
        for (const int tone : {-12, -9, -6, -5, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9})
            for (const bool fourth_subject : {false, true}) {
                Story s;
                fill(s);
                s.full = false; s.kind = kind; s.tone = std::uint32_t(tone); s.subject_fourth = fourth_subject;
                cases.push_back(maker.make(s, name_of(s, "short remainder")));
            }
    // A magazine on a day of kind 8, a club without nation or without id, an outlet in the user's region.
    for (const std::uint32_t outlet : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, none})
        for (const std::uint32_t manager : {0u, 1u, 2u})
            for (const bool eight : {false, true}) {
                Story s;
                fill(s);
                s.kind = 0x2e; s.tone = 0; s.outlet_kind = outlet; s.manager = manager; s.day_eight = eight; s.outlet_region = random.pick<std::uint32_t>({none, 3});
                cases.push_back(maker.make(s, name_of(s, "outlet")));
            }
    for (const std::uint32_t kind : {0x2cu, 0x28u, 0x2eu}) {
        Story s;
        fill(s);
        s.kind = kind; s.club_without_nation = true; s.manager = 1; s.nation_a = none; s.nation_b = none; s.nation = none; s.legend = none;
        cases.push_back(maker.make(s, name_of(s, "no nation")));
        Story t;
        fill(t);
        t.kind = kind; t.club = none; t.manager = 0;
        cases.push_back(maker.make(t, name_of(t, "no club")));
    }
    // Random draws on top.
    for (unsigned draw = 0; draw < 4000; ++draw) {
        Story s;
        fill(s);
        s.full = !random.chance(5);
        s.kind = random.pick<std::uint32_t>({0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x34, 0x35});
        s.tone = random.pick<std::uint32_t>({0, 1, 2, 3, 4, none});
        s.matched = s.kind == 0x34 ? !random.chance(10) : random.chance(8);
        s.fake_competitions = random.chance(4);
        s.day_eight = random.chance(3);
        s.subject_fourth = random.chance(3);
        s.legend_nation = random.chance(3);
        s.manager = random.next() % 6;
        cases.push_back(maker.make(s, name_of(s, "draw")));
    }
    (void)league;
}
