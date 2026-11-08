// MANAGER_MANAGER.CPP checker cases for the manager news text block (manager_manager_news.hpp): 0x101b9b50,
// the news item of a club's manager after a match, and 0x101bfd10, the comment it can end with.
//
// Real clubs, competitions, people and the module object of the career's snapshot; what a case varies is what
// the routines read: the event code (every code of the original's dispatch, the unlisted ones with each bit
// the fallback tests, and 0), the clubs and competition, the eight stack arguments the caller leaves at its
// sp + 0x38 (written with Case::memory at the snapshot's own r1: the triple of person ids in the scratch area,
// the mode word, and the five bytes the comment rules compare), the board's row flags, the club's league link
// and the league record's level and margin bytes. A fixed seed draws the combinations; leaning batches reach
// the rules a uniform draw rarely does. Other modules' routines (the cup test, the club flag, the league's
// method) run as they are in most cases and answer 0/1 in some, so that both outcomes of each test run. The
// direct cases of 0x101bfd10 give it the registers the original's caller holds (r26-r30 real people).
#include "manager_manager_checks.hpp"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace cm::aot::manager_manager_checks {
namespace {
constexpr std::uint32_t compose = 0x101b9b50, comment = 0x101bfd10;
constexpr std::int32_t manager_object = -0x79f0, club_total = -0x7b50, pool_slot = -0x5368, leagues_slot = -0x7b5c,
                       people_slot = -0x7be0;
constexpr std::uint32_t cup_flag = 0x10494d20, club_flag = 0x100c7d00;
constexpr std::uint32_t phrase_area = scratch_base + 0x3200;
constexpr std::uint32_t ids_area = scratch_base + 0x3000, link_area = scratch_base + 0x3100,
                        text_area = scratch_base + 0x4000;
struct Rng {
    std::uint64_t state;
    std::uint32_t next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return std::uint32_t(state >> 33);
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};
// Every event code the original's dispatch (0x101b9c0c..0x101b9ff4) sends to a text of its own.
constexpr std::uint32_t listed_codes[] = {
    0x401, 0x402, 0x408, 0x420, 0x441, 0x442, 0x444, 0x448, 0x450, 0x460, 0x481, 0x482, 0x484, 0x488, 0x4a0,
    0x501, 0x502, 0x541, 0x542, 0x544, 0x581, 0x582, 0x584, 0x608, 0x620, 0x648, 0x650, 0x660, 0x688, 0x6a0,
    0x808, 0x810, 0x820, 0x848, 0x850, 0x860, 0x888, 0x890, 0x8a0, 0x948, 0x988, 0xa08, 0xa10, 0xa20, 0xa48,
    0xa50, 0xa60, 0xa88, 0xa90, 0xaa0, 0x1001, 0x1002, 0x1008, 0x1020, 0x1041, 0x1042, 0x1044, 0x1048, 0x1060,
    0x1081, 0x1082, 0x1084, 0x1088, 0x10a0, 0x1101, 0x1102, 0x1120, 0x1141, 0x1142, 0x1144, 0x1148, 0x1160,
    0x1181, 0x1182, 0x1184, 0x1188, 0x11a0, 0x1208, 0x1220, 0x1248, 0x1260, 0x1288, 0x12a0,
};
constexpr std::uint32_t row_flag_bits[] = {1, 2, 4, 8, 0x40, 0x20};
std::string hex(std::uint32_t value) {
    static const char digits[] = "0123456789abcdef";
    std::string text;
    do text.insert(text.begin(), digits[value & 15]); while (value >>= 4);
    return text;
}

// Whether a guest address reads (the club-flag answer below only goes to clubs whose region text exists).
bool readable(const Scenario& sc, std::uint32_t at) {
    std::uint8_t byte[4];
    return sc.s.memory.try_copy_unobserved(at, byte, sizeof byte);
}

struct World {
    std::uint32_t object, rows, clubs, people, people_table, leagues;
    std::vector<unsigned> managed;       // clubs with a manager (the news is about one)
    std::vector<unsigned> league_ids;    // ids with a league record that has a vtable
    std::vector<unsigned> missing_ids;   // ids with no league record
};
World world_of(const Scenario& sc) {
    World w{};
    w.object = sc.global(manager_object);
    w.rows = sc.r32(w.object);
    w.clubs = sc.global(club_total);
    w.people = sc.people();
    w.people_table = sc.global(people_slot);
    w.leagues = sc.global(leagues_slot);
    for (unsigned n = 0; n < w.clubs; ++n)
        if (sc.r32(sc.club(n) + 0x90)) w.managed.push_back(n);
    for (unsigned id = 0; id < 80; ++id) {
        const auto record = sc.r32(w.leagues + id * 4);
        if (!record) w.missing_ids.push_back(id);
        else if (sc.r32(record)) w.league_ids.push_back(id);
    }
    return w;
}

// How a batch leans (percentages).
struct Lean {
    unsigned listed = 80, no_competition = 3, comment_code = 70, kind_two = 30, kind_minus_two = 30, flagged = 85,
             league_path = 50, cup_answer = 40, club_answer = 40, ids_present = 55;
};

// The eight stack arguments of 0x101b9b50 and the triple of ids they point to.
void stack_arguments(Rng& rng, const Scenario& sc, const World& w, const Lean& lean, Case& item, bool force_kind = false) {
    const auto sp = std::uint32_t(sc.s.r<4>(4));
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    // Where the first slot is absent the name routine is handed the module object (with the flag) or the caller's
    // r28 (the phrase, which is no person): the direct cases of 0x101bfd10 cover those, so here it is always present.
    for (unsigned slot = 0; slot < 3; ++slot) {
        const auto id = rng.chance(slot == 0 ? 100 : lean.ids_present) ? rng.below(w.people) : 0xffffffffu;
        put(4, ids_area + slot * 4, id);
    }
    for (unsigned k = 0; k < 8; ++k) put(1, phrase_area + k, 'a' + k);  // the phrase text (a scratch string)
    put(1, phrase_area + 8, 0);
    put(4, sp + 0x38, rng.chance(95) ? ids_area : 0);
    put(4, sp + 0x3c, rng.pick<std::uint32_t>({0, 1, 2, 3, 0xffffffffu, 5, 0x80000000u, 0x7fffffffu, 1, 0, 2}));
    const auto kind = [&]() -> std::uint32_t {
        const auto draw = rng.below(100);
        if (draw < lean.kind_two) return 2;
        if (draw < lean.kind_two + lean.kind_minus_two) return 0xfe;
        return rng.pick<std::uint32_t>({0, 1, 3, 0xff, 0x7f, 0x80, 0xfd, 0x102, 0x2fe});
    };
    put(1, sp + 0x47, force_kind ? 0xfe : kind());
    // expected, actual, limit: around each other, so every comparison's both sides run.
    const int expected = rng.chance(10) ? int(rng.below(256)) - 128 : int(rng.below(11)) - 5;
    const auto near = [&](int base) {
        return std::uint32_t(rng.chance(8) ? int(rng.below(256)) - 128
                                          : base + rng.pick<int>({-9, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 9, -4, -3, 3, 2, 5}));
    } ;
    put(1, sp + 0x4b, std::uint32_t(expected));
    put(1, sp + 0x4f, near(expected));
    put(1, sp + 0x53, near(expected));
    put(1, sp + 0x57, rng.chance(lean.flagged) ? rng.pick<std::uint32_t>({1, 1, 1, 0xff, 0x80, 0x7f, 5}) : rng.pick<std::uint32_t>({0, 0x100, 0x200}));
}

// The board's row, the club's league link and the league record the comment's -2 rule reads.
void league_fields(Rng& rng, const Scenario& sc, const World& w, const Lean& lean, std::uint32_t club, Case& item) {
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    const auto row = w.rows + sc.r32(club) * 0x4c;
    put(2, row + 0xe, rng.chance(60) ? row_flag_bits[rng.below(6)] | (rng.chance(30) ? 0x20u : 0u) : rng.pick<std::uint32_t>({0, 0x20, 0x21, 0x60, 0xffff, 0x400, 0x8000}));
    if (rng.chance(lean.league_path) && !w.league_ids.empty()) {
        put(4, club + 0x30, rng.chance(94) ? link_area : 0);
        const auto missing = rng.chance(10) && !w.missing_ids.empty();
        const auto id = missing ? w.missing_ids[rng.below(unsigned(w.missing_ids.size()))] : w.league_ids[rng.below(unsigned(w.league_ids.size()))];
        put(4, link_area, id);
        if (!missing) {
            const auto record = sc.r32(w.leagues + id * 4);
            put(2, record + 0x36, rng.pick<std::uint32_t>({0, 1, 2, 3, 5, 0xfffe, 0xfffd, 7, 0x81, 0x7f, 4, 0xff, 0x100, 0xffff}));
            put(1, record + 0xc3, rng.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 0xff, 0xfe, 0x7f, 0x80, 10}));
        }
    }
}

Case compose_case(Rng& rng, const Scenario& sc, const World& w, const Lean& lean) {
    Case item;
    item.entry = compose;
    const auto club_n = w.managed[rng.below(unsigned(w.managed.size()))];
    auto opponent_n = rng.chance(5) ? club_n : rng.below(w.clubs);
    const auto club = sc.club(club_n), opponent = sc.club(opponent_n);
    const bool no_competition = rng.chance(lean.no_competition);
    const auto competition = no_competition ? 0u : sc.competition(rng.below(60));
    std::uint32_t code;
    if (rng.chance(lean.listed)) code = listed_codes[rng.below(unsigned(sizeof listed_codes / sizeof *listed_codes))];
    else if (rng.chance(40)) code = rng.pick<std::uint32_t>({0, 1, 2, 4, 0x400, 0x800, 0xc00, 0x1000, 0x1800, 0x1c00, 0x7ff, 0xfff, 0x2000, 0x403, 0x583, 0x483, 0x809, 0x1183, 0x1288 + 1});
    else code = rng.next() & (rng.chance(50) ? 0x1fff : 0xffff);
    if (rng.chance(lean.comment_code) && !rng.chance(20)) code |= rng.pick<std::uint32_t>({0x800, 0x1000, 0x1800});
    const auto phrase = rng.chance(75) ? sc.toc(pool_slot) + 0x13362 : phrase_area;
    item.registers = {{3, w.object}, {4, club}, {5, opponent}, {6, competition}, {7, code}, {8, phrase}};
    stack_arguments(rng, sc, w, lean, item);
    league_fields(rng, sc, w, lean, club, item);
    if (rng.chance(lean.cup_answer)) item.answers.push_back({cup_flag, rng.chance(50) ? 1u : 0u});
    if (rng.chance(lean.club_answer)) {
        const auto region = sc.r32(club + 0x2c);
        const bool region_text = region != 0 && region != 0xffffffffu && readable(sc, region) && readable(sc, region + 0x100);
        item.answers.push_back({club_flag, region_text && rng.chance(50) ? 1u : 0u});
    }
    item.label = "compose club " + std::to_string(club_n) + " vs " + std::to_string(opponent_n) + " code " + hex(code) +
                 (competition ? "" : " no competition");
    return item;
}

// A direct call of 0x101bfd10: the people the caller leaves in r26-r30, r3 (the object or a person).
Case comment_case(Rng& rng, const Scenario& sc, const World& w, unsigned n) {
    Case item;
    item.entry = comment;
    auto put = [&](unsigned size, std::uint32_t at, std::uint32_t value) { item.memory.push_back({size, at, value}); };
    const auto person = [&] { return sc.person(rng.below(w.people)); };
    const auto absent = [&] { return rng.chance(35) ? 0xffffffffu : rng.below(w.people); };
    std::uint32_t slots[3] = {absent(), absent(), absent()};
    if (n % 7 == 0) slots[0] = slots[1] = slots[2] = 0xffffffffu;
    else if (n % 7 == 1) slots[0] = 0xffffffffu;
    else if (n % 7 == 2) slots[0] = slots[1] = 0xffffffffu;
    for (unsigned k = 0; k < 3; ++k) put(4, ids_area + 0x40 + k * 4, slots[k]);
    const bool no_record = n % 23 == 22;
    const auto flag = rng.pick<std::uint32_t>({0, 1, 1, 0xff, 0x80, 0x100, 0, 5});
    const auto mode = rng.pick<std::uint32_t>({0, 1, 2, 3, 0xffffffffu, 5, 0x80000000u, 0x7fffffffu, 1, 0, 2, 0xfffffffeu});
    item.registers = {{3, rng.chance(75) ? person() : w.object}, {4, no_record ? 0u : ids_area + 0x40}, {5, flag}, {6, text_area + 0x400 * (n % 4)},
                      {7, mode}, {26, person()}, {27, person()}, {28, person()}, {29, person()}, {30, person()}};
    item.label = "comment " + std::to_string(n) + " flag " + hex(flag) + " mode " + hex(mode) + (no_record ? " no record" : "");
    return item;
}
}

MANAGER_MANAGER_CASES("news compose") {
    if (scenario.match || !scenario.global(club_total)) return;
    const auto w = world_of(scenario);
    if (w.clubs < 3 || w.people < 4 || w.league_ids.empty()) return;
    Rng rng{0x6e3571ce};
    for (unsigned k = 0; k < 1200; ++k) cases.push_back(compose_case(rng, scenario, w, {}));
    // The comment rules: a code that asks for one, kind 2 or -2, the league path.
    for (unsigned k = 0; k < 500; ++k) cases.push_back(compose_case(rng, scenario, w, {.listed = 95, .no_competition = 0, .comment_code = 100, .kind_two = 20, .kind_minus_two = 70, .flagged = 100, .league_path = 85, .cup_answer = 50, .club_answer = 50, .ids_present = 70}));
    for (unsigned k = 0; k < 300; ++k) cases.push_back(compose_case(rng, scenario, w, {.listed = 95, .no_competition = 0, .comment_code = 100, .kind_two = 80, .kind_minus_two = 10, .flagged = 100, .league_path = 20, .cup_answer = 0, .club_answer = 0}));
    // Unlisted codes and the missing competition.
    for (unsigned k = 0; k < 250; ++k) cases.push_back(compose_case(rng, scenario, w, {.listed = 5, .no_competition = 10, .comment_code = 30, .cup_answer = 0, .club_answer = 60}));
    // Every listed code once with the cup test answered each way.
    for (const auto code : listed_codes)
        for (unsigned answer = 0; answer < 2; ++answer) {
            auto item = compose_case(rng, scenario, w, {.no_competition = 0, .cup_answer = 0, .club_answer = 0});
            item.registers[4] = {7, code};
            item.answers.push_back({cup_flag, answer});
            item.label = "compose code " + hex(code) + " cup " + std::to_string(answer);
            cases.push_back(item);
        }
}

MANAGER_MANAGER_CASES("news comment") {
    if (scenario.match || !scenario.global(club_total)) return;
    const auto w = world_of(scenario);
    if (w.clubs < 3 || w.people < 4) return;
    Rng rng{0x5eed2c0d};
    for (unsigned k = 0; k < 700; ++k) cases.push_back(comment_case(rng, scenario, w, k));
}
}
