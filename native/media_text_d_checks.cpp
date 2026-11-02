// Cases for the press story writer: story records laid out in the scratch area over real clubs, staff and
// competitions. A story is chosen by its code (field 9), the result kind and numbers that pick wordings, so
// every code gets its own batch of records with those numbers drawn from small sets (a fixed seed), and the
// structured cases first for what depends on the user's manager (the outlet) and on the match record.
#include "media_checks.hpp"
#include <array>
#include <stdexcept>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t record_at = scratch_base + 0x100, output_at = scratch_base + 0x2000;
constexpr std::uint32_t none = 0xffffffffu;
constexpr std::uint32_t entry = 0x1068a470, fake_table = scratch_base + 0xa000;

void put32(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({4, at, value}); }

// A story record: the word at +8 + 4 * index.
Writes fields(std::uint32_t at, const std::vector<std::uint32_t>& values) {
    Writes out;
    std::uint32_t index = 0;
    for (const auto value : values) put32(out, at + 8 + 4 * index++, value);
    return out;
}
// The text object's own routines answer on both sides: the object is not under test.
const std::vector<std::pair<std::uint32_t, std::uint32_t>> text_answers{{0x1074b830, 0}, {0x1074b960, 0}, {0x1074bab0, 0}};

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

// splitmix64: the draws are fixed, so the cases are the same on every run.
struct Random {
    std::uint64_t state;
    std::uint32_t next() {
        state += 0x9e3779b97f4a7c15ull;
        auto z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return std::uint32_t((z ^ (z >> 31)) >> 16);
    }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + next() % values.size()); }
};

// How the user's manager relates to the club's country: no manager, same country through his club, through
// himself, or another country; and whether the league setting matches. null_country: the club has no
// country and neither has the manager (needs the null page, for the league setting is read through it).
enum class Nation { none, same_by_club, same_by_manager, differs, null_country };

struct Story {
    std::uint32_t full = 1, code = 0x50, kind = 0, outlet = 1, club = 7, competition = 3, match_competition = 3, mood = 2, f5 = 0, f6 = 0;
    bool other_named = true, staff_named = true, has_staff = true, has_match = true;
    std::uint32_t rival = 11, goals_one = 2, goals_two = 1, stage_a = 1, stage_b = 2, home = 1;
    std::uint32_t counted[10] = {0, 1, 5, 6, 12345, 9, 3, 0, 1, 2};
    Nation nation = Nation::none;
    bool same_league = false, exists = false, magazine = false, fake = false, own = false;
    bool flagged = false;  // null_country: the club's first word makes the club-flag routine (0x100c7d00) answer 1
    unsigned stage_text = 0;  // 1, 2: the stage text 0x104769d0 builds is exactly the friendly or the reserve name (its text is in the game's language, so it never is; the call is answered)
    // own: the user's manager is the other person of the story;  // fake: the competition table is four rows, two with the friendly and reserve stage names
    std::string label;
};
}

MEDIA_CASES("text_d") {
    const auto staff = named_staff(scenario, 4);
    if (staff.size() < 4) throw std::runtime_error("text_d cases: no named staff in the snapshot");
    const auto user_base = scenario.toc(-0x7bb0);
    const auto user_row = user_base + std::uint32_t(std::int32_t(std::int16_t(scenario.r32(user_base + 0x2c58) >> 16))) * 0x2c0;
    const auto staff_table = scenario.global(-0x7be0);
    const auto user_manager = scratch_base + 0x8000, manager_club = scratch_base + 0x8100;
    // The writer's frame (0x2f50 bytes below the entry stack pointer) holds the stage text at +0x304.
    const std::uint32_t stage_buffer = std::uint32_t(scenario.s.r<4>(4)) - 0x2f50 + 0x304;
    const auto world_setting = scenario.r32(scenario.toc(-0x7a10));
    constexpr std::uint32_t day_kind_byte = 0x108bba90 - 0x35ac;
    // Clubs with a country (one without faults in every path, on both sides).
    std::vector<std::uint32_t> clubs_with_nation;
    for (std::uint32_t index = 1; index < 300; ++index) {
        if (scenario.r32(scenario.club(index) + 0x2c)) { if (clubs_with_nation.size() < 40) clubs_with_nation.push_back(index); }
    }

    // The friendly and reserve stages (found by the name text at +0x4d), and others.
    std::uint32_t friendly = 1, reserve = 2;
    for (std::uint32_t index = 1; index < 300; ++index) {
        try {
            std::string name;
            for (unsigned k = 0; k < 12; ++k) name += char(scenario.s.memory.read<1>(scenario.competition(index) + 0x4d + k));
            if (name.rfind("Friendly", 0) == 0) friendly = index;
            if (name.rfind("Reserve", 0) == 0) reserve = index;
        } catch (const std::exception&) { break; }
    }

    const auto make = [&](const Story& s) {
        const auto& other = staff[0];
        const auto& subject = staff[1];
        std::vector<std::uint32_t> v{0, s.kind, s.outlet, 0, s.club, s.f5, s.f6, s.competition, 0, s.code, s.mood,
                                     other.index, s.other_named ? other.first : other.first + 1, other.middle, other.last,
                                     s.has_staff ? subject.index : none, s.staff_named ? subject.first : subject.first + 1, subject.middle, subject.last,
                                     s.has_match ? 1u : 0u};
        if (s.has_match) {
            const std::vector<std::uint32_t> match{150, 2001, s.match_competition, s.stage_a, s.stage_b, 4, s.home ? s.club : s.rival,
                                                    s.home ? s.rival : s.club, s.goals_one, s.goals_two, 5};
            v.insert(v.end(), match.begin(), match.end());
        }
        v.insert(v.end(), std::begin(s.counted), std::end(s.counted));
        auto memory = fields(record_at, v);
        const auto home_nation = scenario.r32(scenario.club(s.club) + 0x2c);
        switch (s.nation) {
        case Nation::none: put32(memory, user_row + 4, 0); break;
        case Nation::same_by_club:
            put32(memory, user_row + 4, user_manager);
            put32(memory, user_manager + 0x24, manager_club);
            put32(memory, manager_club + 0x2c, home_nation);
            if (home_nation) put32(memory, home_nation + 0x68, s.same_league ? world_setting : world_setting + 1);
            break;
        case Nation::same_by_manager:
            put32(memory, user_row + 4, user_manager);
            put32(memory, user_manager + 0x24, 0);
            put32(memory, user_manager + 0xc, home_nation);
            if (home_nation) put32(memory, home_nation + 0x68, s.same_league ? world_setting : world_setting + 1);
            break;
        case Nation::null_country:
            // Manager and club both without a country: the nations are the same (null), and the league
            // setting is what lives at address 0x68. The club flag answers 1 for a first word of 0x7fffffff.
            put32(memory, scenario.club(s.club) + 0x2c, 0);
            put32(memory, scenario.club(s.club), s.flagged ? 0x7fffffffu : 0u);
            // The user's selected slot is row 1, so the row offset the manager path leaves in r5 (0x2c0) shows
            // against the club id's sign that the original leaves there after the club-flag test (0).
            memory.push_back({2, user_base + 0x2c58, 1});
            put32(memory, user_base + 0x2c0 + 4, user_manager);
            put32(memory, user_manager + 0x24, 0);
            put32(memory, user_manager + 0xc, 0);
            put32(memory, 0x68, s.same_league ? world_setting : world_setting + 1);
            break;
        case Nation::differs:
            put32(memory, user_row + 4, user_manager);
            put32(memory, user_manager + 0x24, 0);
            put32(memory, user_manager + 0xc, home_nation + 4);
            break;
        }
        if (s.own) put32(memory, user_row + 4, staff_table + other.index * 0x70);
        if (s.fake) {
            // Four rows: the real row 3 with the friendly and reserve stage names written over it, then rows 3 and 87 as they are.
            const char* names[2]{"Friendly", "Reserve"};
            for (unsigned row = 0; row < 4; ++row) {
                const auto at = fake_table + row * 0x6c;
                const auto source = row == 3 ? scenario.competition(87) : scenario.competition(3);
                for (unsigned word = 0; word < 0x6c; word += 4) put32(memory, at + word, scenario.r32(source + word));
                if (row < 2)
                    for (const unsigned offset : {0x1au, 0x4du})
                        for (unsigned k = 0; k < 20; ++k) memory.push_back({1, at + offset + k, k < std::string(names[row]).size() ? std::uint32_t(names[row][k]) : 0u});
            }
            put32(memory, scenario.toc(-0x7bbc), fake_table);
        }
        if (s.magazine) memory.push_back({1, day_kind_byte, 8});
        auto answers = text_answers;
        answers.push_back({0x1029eb20, s.exists ? 1u : 0u});
        if (s.stage_text) {
            // The stage text is the frame's buffer at +0x304, which the answered call leaves as it finds it.
            const std::string text = s.stage_text == 1 ? "Friendly" : "Reserve";
            for (unsigned k = 0; k <= text.size(); ++k) memory.push_back({1, stage_buffer + k, k < text.size() ? std::uint32_t(text[k]) : 0u});
            answers.push_back({0x104769d0, 0x47});
        }
        std::string label = "story " + std::to_string(s.code) + " full=" + std::to_string(s.full) + " kind=" + std::to_string(int(s.kind)) + " outlet=" + std::to_string(int(s.outlet)) + " club=" + std::to_string(s.club) + " nation=" + std::to_string(int(s.nation)) + (s.same_league ? " league" : "") + " " + s.label;
        return Case{.entry = entry, .registers = {{3, 0}, {4, record_at}, {5, s.full}, {6, output_at}}, .memory = std::move(memory), .f1 = {},
                    .label = std::move(label), .answers = std::move(answers)};
    };

    Random random{0x7e57d};
    const auto club_pick = [&] { return clubs_with_nation[random.next() % clubs_with_nation.size()]; };
    const auto draw = [&](std::uint32_t code, std::uint32_t full) {
        Story s;
        s.full = full; s.code = code;
        s.kind = random.pick<std::uint32_t>({0, 0, 1, 1, 2, 5, none});
        s.outlet = random.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 6, 7, none});
        s.club = club_pick();
        s.competition = random.pick<std::uint32_t>({1, 2, 3, 5, 9, 14, friendly, reserve});
        s.match_competition = random.pick<std::uint32_t>({friendly, reserve, 3, 3, 5, 9, 14, 87});
        s.mood = random.pick<std::uint32_t>({none, 0, 1, 2, 3, 4, 5});
        s.f5 = random.pick<std::uint32_t>({0, 1, 2}); s.f6 = random.pick<std::uint32_t>({0, 1, 2});
        s.other_named = random.next() % 4 != 0; s.staff_named = random.next() % 4 != 0; s.has_staff = random.next() % 4 != 0;
        s.has_match = random.next() % 5 != 0;
        s.rival = club_pick(); s.home = random.next() % 2;
        const auto goals = random.pick<std::pair<std::uint32_t, std::uint32_t>>({{2, 1}, {1, 2}, {1, 1}, {0, 0}, {5, 1}, {1, 5}, {4, 3}, {3, 4}, {3, 3}, {6, 0}, {0, 6}, {0xfe, 1}, {2, 0}, {0, 2}});
        s.goals_one = goals.first; s.goals_two = goals.second;
        s.stage_a = random.pick<std::uint32_t>({0, 0, 1, 2}); s.stage_b = random.pick<std::uint32_t>({0, 1, 2, 3});
        s.counted[0] = random.pick<std::uint32_t>({0, 1, 2, 3, 5, 8, none});
        s.counted[1] = random.pick<std::uint32_t>({0, 1, 2});
        // Persons' nationalities are compared with the club's country: sometimes make them equal.
        const auto nation_id = scenario.r32(scenario.r32(scenario.club(s.club) + 0x2c) + 0x68);
        s.counted[2] = random.pick<std::uint32_t>({0, 5, 17, 140, 399, none, nation_id, nation_id});
        s.counted[3] = random.pick<std::uint32_t>({0, 6, 18, 141, 398, none, none, nation_id, nation_id});
        s.counted[4] = random.pick<std::uint32_t>({0, 5000, 12345678});
        s.counted[5] = random.pick<std::uint32_t>({none, 3, 12, 25, 30});
        s.counted[6] = random.pick<std::uint32_t>({none, 0, 1, 2, 9, 20});
        s.counted[7] = random.pick<std::uint32_t>({none, 0, 1, 2});
        s.counted[8] = random.pick<std::uint32_t>({none, 0, 1, 2, 3, 4, 5, 8});
        s.counted[9] = random.pick<std::uint32_t>({none, 0, 1, 2, 3, 5, 9});
        s.nation = random.pick<Nation>({Nation::none, Nation::same_by_club, Nation::same_by_club, Nation::same_by_manager, Nation::differs});
        s.same_league = random.next() % 3 != 0;
        s.exists = random.next() % 2; s.magazine = random.next() % 2;
        s.fake = random.next() % 3 == 0;
        s.own = random.next() % 4 == 0;
        s.stage_text = random.pick<unsigned>({0, 0, 1, 2});
        if (random.next() % 40 == 0) s.club = none;
        if (s.fake) { s.competition = random.next() % 4; s.match_competition = random.next() % 4; }
        s.label = "draw";
        return s;
    };

    // The codes the writer knows, in the order it tests them, and two it does not.
    constexpr std::uint32_t codes[]{0x50, 0x51, 0x3f, 0x5f, 0x60, 0x42, 0x43, 0x44, 0x5a, 0x5c, 0x5b, 0x52, 0x3e, 0x5e, 0x3c, 0x3d,
                                    0x63, 0x61, 0x5d, 0x62, 0x40, 0x41, 0x10, 0x70};
    // The longest story chooses its closing line by the kind, the goals and a number: every combination once.
    for (const std::uint32_t kind : {0u, 1u, 2u, 5u, none})
        for (const std::uint32_t closing : {1u, 2u, 3u, 4u, 5u})
            for (const bool level : {true, false})
                for (const bool staffed : {true, false})
                for (const bool staged : {true, false})
                for (const unsigned stage : {0u, 1u, 2u}) {
                    auto s = draw(0x3d, 1);
                    s.kind = kind; s.counted[8] = closing; s.has_match = true; s.stage_text = stage; s.stage_a = staged ? 1 : 0; s.stage_b = 0;
                    s.goals_one = 2; s.goals_two = level ? 2 : 1; s.has_staff = staffed;
                    s.fake = false; s.label = "closing";
                    cases.push_back(make(s));
                }
    // The club-flag branch of the outlet choice (the manager's country and the league setting both match with
    // no country at all): the flag routine answers or not, and the club id and the code have the same parity or not.
    bool null_page = true;
    try { scenario.r32(0x68); } catch (const std::exception&) { null_page = false; }
    if (null_page)
        for (const bool flagged : {true, false})
            for (const bool same_league : {true, false})
                for (const std::uint32_t code : {0x50u, 0x51u})
                    for (const std::uint32_t outlet : {1u, 3u, 5u})
                        for (const unsigned parity : {0u, 1u}) {
                            auto s = draw(code, 1);
                            s.nation = Nation::null_country; s.flagged = flagged; s.same_league = same_league; s.outlet = outlet;
                            s.counted[9] = 2; s.kind = 0; s.own = false;
                            s.club = clubs_with_nation[parity];  // (replaced below by the first club of the wanted parity)
                            for (const auto candidate : clubs_with_nation) if (candidate % 2 == parity) { s.club = candidate; break; }
                            s.fake = false; s.label = "club flag";
                            cases.push_back(make(s));
                        }
    for (const auto code : codes) {
        // The long stories have the most paths.
        const unsigned draws = code == 0x3d ? 2600 : code == 0x5a || code == 0x5c ? 1000 : code == 0x44 ? 600 : 300;
        for (unsigned i = 0; i < draws; ++i) cases.push_back(make(draw(code, 1)));
        // The stories with a player in them check that the staff member, the club and the player are all named.
        for (const bool staffed : {true, false})
            for (const std::uint32_t flag : {none, 0u, 1u})
                for (const std::uint32_t club_field : {none, 12u})
                    for (const std::uint32_t player_field : {none, 3u})
                        for (const std::uint32_t kind : {0u, 1u, 2u}) {
                            auto s = draw(code, 1);
                            s.has_staff = staffed; s.counted[7] = flag; s.counted[5] = club_field; s.counted[6] = player_field; s.kind = kind;
                            s.label = "named";
                            cases.push_back(make(s));
                        }
        for (unsigned i = 0; i < 24; ++i) cases.push_back(make(draw(code, 0)));
    }
}
