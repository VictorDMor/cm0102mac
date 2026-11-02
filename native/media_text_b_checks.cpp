// Cases for the board and fans story text: story records laid out in the scratch area over real
// clubs and staff. The stories are chosen by many independent numbers, so the cases draw them
// from the values the text tests (a fixed seed), with the structured cases for the paths that
// depend on the people's records first.
#include "media_checks.hpp"
#include <array>
#include <stdexcept>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t record_at = scratch_base + 0x100, output_at = scratch_base + 0x2000;
constexpr std::uint32_t none = 0xffffffffu;

void put32(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({4, at, value}); }
void put16(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({2, at, value}); }
void put8(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({1, at, value}); }

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

// Everything a story is chosen by.
struct Story {
    bool full = true, board = true;
    std::uint32_t code = 0x21, tone = 1, seed = 0, club = 7, kind = 4, count = 5;
    bool subject = true, chairman = true, own_manager = false, named = true;
    std::uint32_t flags = 0;  // 0: the record has none
    std::uint32_t note_a = 0, note_b = 0, note_c = 0, extra_a = 0, extra_b = 0, last = 0;
    std::uint32_t chair_rating = 0;  // the chairman's club's byte: with the count it decides whether he is involved
    bool chairman_club = true;
    std::uint32_t club_first_word = 7, club_grade = 0, nation = 0;  // nation: 0 none, 1 no next, 2 other, 3 same
    std::uint32_t staff_slots = 0;     // which of the club's staff slots (+0x5c...+0x70) hold the subject: bitmask
    std::uint32_t subject_club = 1, subject_club_44 = 0, subject_club_1c = 0, b63 = 0, b65 = 0, b66 = 0, b67 = 0;
};

struct Maker {
    const Scenario& scenario;
    std::vector<Staff> staff;
    std::uint32_t user_row;
    Case make(const Story& s, const std::string& label) const {
        const auto& manager = staff[0];
        const auto& subject = staff[1];
        const auto& chairman = staff[2];
        const auto tweak = [&](std::uint32_t id) { return s.named ? id : id + 1; };
        std::vector<std::uint32_t> v{s.board ? 1u : 0u, s.code, s.tone, s.seed, s.club, s.kind, manager.index, tweak(manager.first), manager.middle, manager.last};
        if (s.subject) v.insert(v.end(), {subject.index, tweak(subject.first), subject.middle, subject.last, 77});
        else v.push_back(none);
        if (s.chairman) v.insert(v.end(), {chairman.index, tweak(chairman.first), chairman.middle, chairman.last});
        else v.push_back(none);
        v.push_back(s.count);
        v.push_back(s.flags ? 1 : 0);
        if (s.flags) v.push_back(s.flags);
        v.insert(v.end(), {s.note_a, s.note_b, s.note_c, s.extra_a, s.extra_b, 99, s.last});
        Writes memory;
        for (std::size_t n = 0; n < v.size(); ++n) put32(memory, record_at + 8 + 4 * std::uint32_t(n), v[n]);
        const auto row = [&](const Staff& member) { return scenario.global(-0x7be0) + member.index * 0x70; };
        const auto club_row = scenario.club(s.club);
        const auto chairman_club = scratch_base + 0x9000, subject_club = scratch_base + 0x9100, nation_at = scratch_base + 0x9200,
                   next_at = scratch_base + 0x9300;
        put32(memory, row(chairman) + 0x1c, s.chairman_club ? chairman_club : 0);
        put8(memory, chairman_club + 0x36, s.chair_rating);
        put32(memory, row(subject) + 0x14, s.subject_club ? subject_club : 0);
        put8(memory, subject_club + 0x44, s.subject_club_44);
        put8(memory, subject_club + 0x1c, s.subject_club_1c);
        put8(memory, row(subject) + 0x63, s.b63);
        put8(memory, row(subject) + 0x65, s.b65);
        put8(memory, row(subject) + 0x66, s.b66);
        put8(memory, row(subject) + 0x67, s.b67);
        put32(memory, club_row, s.club_first_word);
        put16(memory, club_row + 0x1ec, s.club_grade);
        constexpr std::uint32_t slots[]{0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70};
        for (unsigned n = 0; n < 6; ++n) put32(memory, club_row + slots[n], (s.staff_slots >> n & 1) ? row(subject) : 0);
        put32(memory, club_row + 0x2c, s.nation ? nation_at : 0);
        put32(memory, nation_at + 0x74, s.nation >= 2 ? next_at : 0);
        put32(memory, next_at + 8, scenario.global(-0x7a70) + (s.nation == 2 ? 4 : 0));
        put32(memory, user_row + 4, s.own_manager ? scenario.global(-0x7be0) + manager.index * 0x70 : 0);
        return {.entry = 0x1066c670, .registers = {{3, 0}, {4, record_at}, {5, s.full ? 1u : 0u}, {6, output_at}}, .memory = std::move(memory), .f1 = {},
                .label = label, .answers = text_answers};
    }
};

// A small deterministic generator for the sampled cases.
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

MEDIA_CASES("text_b") {
    auto staff = named_staff(scenario, 3);
    if (staff.size() < 3) throw std::runtime_error("text_b cases: no named staff in the snapshot");
    const auto user_base = scenario.toc(-0x7bb0);
    const auto user_row = user_base + std::uint32_t(std::int32_t(std::int16_t(scenario.r32(user_base + 0x2c58) >> 16))) * 0x2c0;
    const Maker maker{scenario, staff, user_row};
    Random random{0x7e57b0a2d};
    // A chairman who is involved (his club's byte mod 20 is at least the count mod 5), in a short story.
    {
        Story s;
        s.full = false; s.board = true; s.code = 0x29; s.tone = 1; s.kind = 4; s.count = 4; s.chair_rating = 19;
        cases.push_back(maker.make(s, "short board, transfer, chairman involved"));
    }
    // The codes the tests tell apart, with the ones between and around them.
    const std::vector<std::uint32_t> codes{0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 0x15, 0x21, 0x22, 0x29, 0x2a, 0x28, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x35, 0x36,
                                           0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x44, 0x45, 0x52, 0x5a, 0x5f, 0x60, 0x63, 0x64, none};
    const std::vector<std::uint32_t> kinds{0, 1, 2, 3, 4, 5, 6, 7, 9, 10};
    const std::vector<std::uint32_t> flag_values{0, 0, 1, 2, 4, 8, 0x10, 0x400, 0x401, 0x402, 0x404, 0x408, 0x410, 0x41f, 0x3ff};
    const std::vector<std::uint32_t> counts{0, 1, 2, 3, 4, 5, 7, 0x109a - 1, 0x109a, 0x109b, 0x1d4c, 0x1d4d, 0x251c, 0x251d, 0x2600, 0x3000, 0x3001, 0x3002, none, 0xfffffffeu};
    const std::vector<std::uint32_t> clubs{7, 8, 11, 14, 21, 28, 5};
    // Everything the choice does not depend on gets a random value.
    const auto fill = [&](Story& s) {
        s.club = clubs[random.next() % clubs.size()];
        s.count = counts[random.next() % counts.size()];
        s.seed = random.pick<std::uint32_t>({0, 1, 2, 3, none});
        s.own_manager = random.chance(2);
        s.named = !random.chance(6);
        s.flags = flag_values[random.next() % flag_values.size()];
        s.note_a = random.chance(2);
        s.note_b = random.chance(2);
        s.note_c = random.chance(2);
        s.extra_a = random.chance(2);
        s.extra_b = random.chance(2);
        s.last = random.chance(2);
        s.chair_rating = random.pick<std::uint32_t>({0, 3, 4, 5, 9, 19, 20, 23, 0x80, 0xff});
        s.chairman_club = !random.chance(5);
        s.club_first_word = random.pick<std::uint32_t>({7, 8, 14, 15, 3});
        s.club_grade = random.pick<std::uint32_t>({0, 0x1099, 0x109a, 0x2000});
        s.nation = random.next() % 4;
        s.staff_slots = random.chance(3) ? 0 : 1u << (random.next() % 6);
        s.subject_club = !random.chance(5);
        s.subject_club_44 = random.pick<std::uint32_t>({0, 5, 6, 9, 0xa, 0xf, 0x10, 0x80});
        s.subject_club_1c = random.pick<std::uint32_t>({0, 5, 0xf, 0x10, 0x80});
        s.b63 = random.pick<std::uint32_t>({0, 6, 0xf, 0x10, 0x30});
        s.b65 = random.pick<std::uint32_t>({0, 5, 6, 7, 0x40});
        s.b66 = random.pick<std::uint32_t>({0, 5, 6, 9, 0xa, 0xb, 0x30});
        s.b67 = random.pick<std::uint32_t>({0, 1, 5, 6, 0xa, 0xb, 0x30});
    };
    const auto name_of = [](const Story& s, const char* what, unsigned n) {
        return std::string(what) + " " + std::to_string(n) + " full=" + std::to_string(s.full) + " board=" + std::to_string(s.board) + " code=" + std::to_string(int(s.code)) +
               " tone=" + std::to_string(s.tone) + " kind=" + std::to_string(s.kind) + (s.chairman ? " chairman" : "") + (s.subject ? " subject" : "");
    };
    // Every combination of the numbers that choose the story.
    unsigned n = 0;
    for (const bool full : {false, true})
        for (const bool board : {true, false})
            for (const std::uint32_t code : codes) {
                if (code > 0x64 && code != none) continue;
                for (const std::uint32_t tone : {0u, 1u, 2u, 3u})
                    for (const std::uint32_t kind : {0u, 2u, 3u, 4u, 5u, 9u})
                        for (const bool involved : {false, true})
                            for (const bool subject : {false, true}) {
                                Story s;
                                fill(s);
                                s.full = full; s.board = board; s.code = code; s.tone = tone; s.kind = kind; s.subject = subject;
                                s.chairman = true; s.chairman_club = true;
                                // Involved: the byte mod 20 is at least the count mod 5.
                                s.chair_rating = involved ? 19 : 0;
                                s.count = random.pick<std::uint32_t>({1, 2, 3, 4, 7, 8, 4249, 7501, 9501, 12289});
                                cases.push_back(maker.make(s, name_of(s, "combination", n++)));
                            }
            }
    // The leaves that depend on the manager being the user's, the seed, the chairman and the subject's records.
    struct Records { std::uint32_t slots, b63, b65, b66; };
    constexpr Records plain{0, 0, 0, 0}, rated{0, 0x10, 5, 0}, listed{8, 0, 0, 6};
    for (const std::uint32_t code : {1u, 3u, 6u, 7u, 8u, 9u, 0x28u, 0x2bu})
        for (const bool board : {true, false})
            for (const std::uint32_t tone : {1u, 2u})
                for (const std::uint32_t kind : {0u, 2u, 3u, 4u, 5u, 9u})
                    for (const std::uint32_t seed : {0u, 1u})
                        for (const bool own : {false, true})
                            for (const bool involved : {false, true})
                                for (const auto& records : code >= 0x28 ? std::vector<Records>{plain, rated, listed} : std::vector<Records>{plain}) {
                                    Story s;
                                    fill(s);
                                    s.full = true; s.board = board; s.code = code; s.tone = tone; s.kind = kind; s.seed = seed; s.own_manager = own; s.last = seed;
                                    s.chair_rating = involved ? 19 : 0;
                                    s.count = involved ? 4 : 3;
                                    s.staff_slots = records.slots; s.b63 = records.b63; s.b65 = records.b65; s.b66 = records.b66;
                                    cases.push_back(maker.make(s, name_of(s, "targeted", n++)));
                                }
    // Random draws on top.
    for (unsigned draw = 0; draw < 4000; ++draw) {
        Story s;
        fill(s);
        s.full = random.chance(2);
        s.board = random.chance(2);
        s.code = codes[random.next() % codes.size()];
        s.tone = random.pick<std::uint32_t>({0, 1, 1, 2, 2, 3});
        s.kind = kinds[random.next() % kinds.size()];
        s.subject = !random.chance(5);
        s.chairman = !random.chance(3);
        cases.push_back(maker.make(s, name_of(s, "draw", draw)));
    }
}
