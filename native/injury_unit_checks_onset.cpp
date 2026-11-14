// INJURY.CPP checker cases for the onset block (injury_unit_onset.hpp): the registration of an injury
// (0x10188cc0), its news (0x1018aed0), the settling of pending injuries (0x1018b6e0) and the daily pass
// (0x10185410).
//
// The injury manager is the game's own (global -0x7ba0), and so are its status rows, injury records, the
// people and clubs. What the rules read is set with Case::memory on real people: the morale, proneness,
// resilience and form bytes, the pending type, the person's old injury (real injured people have one).
// The type table is never written. The random draws are steered by writing the game's shared sample table
// itself: the base is 0 and the cursor is put one entry before the start, so the first draw reads
// samples[0] and every later one the next entry (a script of up to a few dozen samples; past it the table's
// own halfwords follow). A case with the cursor at the last entry makes its first draw refill the table
// from the generator, and `nth_answers` on 0x1077b770 answers that redraw.
//
// The career has 101,016 people, so the daily pass's pick of a person goes through the scaled draw
// (a range past 0xffff: a float, a nested 0x101864a0): the cases compute the pick of a sample as that code does and
// choose samples whose pick is a person set up for the branch. Unreached by cases: the scaled draw of the
// registration (its ranges are bytes and halfwords, never past 0xffff: proof in the listing, every range
// register is an extsb/extsh result or a small expression); the real calls and mutations reach what they reach.
#include "guest_abi.hpp"
#include "injury_unit_checks.hpp"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdint>
#include <random>
#include <string>

namespace cm::aot::injury_unit_checks {
namespace {
constexpr std::uint32_t daily = 0x10185410, registration = 0x10188cc0, news = 0x1018aed0, settle = 0x1018b6e0;
constexpr std::uint32_t redraw = 0x1077b770, copy_match = 0x1030d4a0, unlink_news = 0x10748ce0,
                        news_text_a = 0x1018b2b0, news_text_b = 0x1018bdd0,
                        settle_class = 0x1018c2f0;

struct Facts {
    explicit Facts(const Scenario& scenario) : s(scenario) {}
    const Scenario& s;
    std::uint32_t manager = s.global(-0x7ba0);
    std::uint32_t rows = s.r32(manager), records = s.r32(manager + 4);
    std::int32_t count = std::int32_t(s.r32(manager + 8));
    std::uint32_t table_start = s.toc(-0x7b8c), table_last = s.r32(s.toc(-0x7b88));
    std::uint32_t cursor_cell = s.toc(-0x7b84), base_cell = s.toc(-0x7b90);
    std::uint32_t types = s.toc(-0x64d4), today = s.toc(-0x7b80), season = s.toc(-0x7a9c);
    std::uint32_t sp = std::uint32_t(s.s.r<4>(guest::reg::gpr(1)));
    std::uint8_t b(std::uint32_t at) const { return std::uint8_t(s.s.memory.read<1>(at)); }
    std::uint16_t h(std::uint32_t at) const { return std::uint16_t(s.s.memory.read<2>(at)); }
    std::uint32_t row(std::uint32_t index) const { return rows + index * 0x24; }
    std::uint32_t person_index(std::uint32_t person) const { return s.r32(person); }
    std::uint32_t profile(std::uint32_t person) const { return s.r32(person + 0x14); }
    std::uint32_t club(std::uint32_t person) const { return s.r32(person + 0x24); }
    std::int8_t pending_byte(std::uint32_t index) const { return std::int8_t(b(row(index) + 0x14)); }
    bool injured(std::uint32_t index) const { return s.r32(row(index) + 0x10) != 0xffffffff; }
    std::uint32_t record_of(std::uint32_t index) const { return records + s.r32(row(index) + 0x10) * 0x38; }
    bool type_can_pend(std::int8_t type) const { return std::int8_t(b(types + std::uint32_t(std::int32_t(type)) * 12 + 9)) != 0; }
};

using Memory = std::vector<Write>;
void put(Memory& m, unsigned size, std::uint32_t at, std::uint32_t value) { m.push_back({size, at, value}); }

// The sample table scripted: base 0, the first draw reads samples[0].
Memory script(const Facts& f, const std::vector<std::uint32_t>& samples) {
    Memory m;
    put(m, 4, f.base_cell, 0);
    put(m, 4, f.cursor_cell, f.table_start - 2);
    for (std::size_t k = 0; k < samples.size(); ++k) put(m, 2, f.table_start + std::uint32_t(k) * 2, samples[k] & 0xffff);
    return m;
}
void append(Memory& to, const Memory& from) { to.insert(to.end(), from.begin(), from.end()); }

struct Rng {
    std::mt19937 engine;
    std::uint32_t below(std::uint32_t n) { return engine() % n; }
    template <class T> T pick(std::initializer_list<T> values) { return *(values.begin() + below(unsigned(values.size()))); }
};
std::vector<std::uint32_t> random_script(Rng& rng, std::size_t length) {
    std::vector<std::uint32_t> samples;
    for (std::size_t k = 0; k < length; ++k) samples.push_back(rng.below(0x10000));
    return samples;
}

// A person's rules bytes set for a branch.
struct Tweak {
    std::int16_t morale = -1000;      // person +0x5c (halfword), -1000: leave
    int prone = -1000, resilience = -1000, form = -1000, level = -1000, pending_type = -1000, pending_count = -1000;
    int standing = -1000;             // profile +0xc
    int fitness_a = -1000, fitness_b = -1000, fitness_c = -1000;  // profile +0x1b, +0x1d, +0x36
};
Memory tweak_memory(const Facts& f, std::uint32_t person, const Tweak& t) {
    Memory m;
    const auto index = f.person_index(person);
    const auto profile = f.profile(person);
    if (t.morale != -1000) put(m, 2, person + 0x5c, std::uint16_t(t.morale));
    if (profile) {
        if (t.prone != -1000) put(m, 1, profile + 0x2d, std::uint8_t(t.prone));
        if (t.resilience != -1000) put(m, 1, profile + 0x34, std::uint8_t(t.resilience));
        if (t.level != -1000) put(m, 1, profile + 0xf, std::uint8_t(t.level));
        if (t.standing != -1000) put(m, 2, profile + 0xc, std::uint16_t(t.standing));
        if (t.fitness_a != -1000) put(m, 1, profile + 0x1b, std::uint8_t(t.fitness_a));
        if (t.fitness_b != -1000) put(m, 1, profile + 0x1d, std::uint8_t(t.fitness_b));
        if (t.fitness_c != -1000) put(m, 1, profile + 0x36, std::uint8_t(t.fitness_c));
    }
    if (t.form != -1000) put(m, 1, f.row(index) + 0x22, std::uint8_t(t.form));
    if (t.pending_type != -1000) put(m, 1, f.row(index) + 0x14, std::uint8_t(t.pending_type));
    if (t.pending_count != -1000) put(m, 1, f.row(index) + 0x15, std::uint8_t(t.pending_count));
    return m;
}

struct People {
    std::vector<std::uint32_t> healthy_players, injured_players, clubless, staff;
};
People pick_people(const Facts& f) {
    People p;
    const auto people = f.s.people();
    for (std::uint32_t i = 0; i < people; i += 3) {
        const auto person = f.s.person(i);
        if (!f.profile(person)) continue;
        const bool has_club = f.club(person) != 0;
        const auto job = f.b(person + 0x62);
        const bool player_job = job == 0xb || job == 0xf || job == 0xd;
        if (!has_club && p.clubless.size() < 3) p.clubless.push_back(i);
        else if (has_club && !player_job && p.staff.size() < 3) p.staff.push_back(i);
        else if (has_club && player_job && f.injured(i) && p.injured_players.size() < 10) p.injured_players.push_back(i);
        else if (has_club && player_job && !f.injured(i) && p.healthy_players.size() < 14) p.healthy_players.push_back(i);
        if (p.clubless.size() >= 3 && p.staff.size() >= 3 && p.injured_players.size() >= 10 && p.healthy_players.size() >= 14) break;
    }
    return p;
}


struct Params {
    std::uint32_t person = 0, type = 0, starts_later = 0, match = 0, report = 0, news_argument = 0;
    int chance = 0;
    std::uint32_t pending_only = 0;
};
Case registration_case(const Facts& f, const Params& p, Memory memory, const std::string& label) {
    Case c{.entry = registration,
           .registers = {{3, f.manager}, {4, p.type}, {5, p.starts_later}, {6, p.person}, {7, p.match}, {8, p.report},
                         {9, p.news_argument}, {10, std::uint32_t(p.chance)}},
           .memory = std::move(memory), .label = label};
    put(c.memory, 4, f.sp + 0x38, p.pending_only);  // the ninth argument's word, in the caller's frame
    return c;
}

Tweak random_tweak(Rng& rng) {
    Tweak t;
    t.morale = rng.pick<std::int16_t>({-1000, 0, 20, 0x1f, 0x22, 0x25, 0x30, 100});
    t.prone = rng.pick({-1000, -5, 0, 8, 11, 20, 60});
    t.resilience = rng.pick({-1000, -3, 0, 10, 30});
    t.form = rng.pick({-1000, 0x10, 0x30, 0x7f});
    return t;
}

INJURY_UNIT_CASES("onset register") {
    if (scenario.match) return;
    const Facts f(scenario);
    if (std::getenv("ONSET_FACTS")) {
        int max_base = -32768;
        for (unsigned t = 0; t < 0x52; ++t) max_base = std::max(max_base, int(std::int16_t(f.h(f.types + t * 12 + 4))));
        std::fprintf(stderr, "max base_days %d\n", max_base);
        for (unsigned k = 0; k < 12; ++k) {
            const auto ranges = scenario.toc(-0x64d0) + k * 3;
            std::fprintf(stderr, "class %u: %d..%d |", k, int(std::int8_t(f.b(ranges + 1))), int(std::int8_t(f.b(ranges + 2))));
        }
        std::fprintf(stderr, "\n");
        for (unsigned t = 0; t < 0x52; ++t) std::fprintf(stderr, "t%x:c%d,p%d ", t, f.b(f.types + t * 12 + 1), int(std::int8_t(f.b(f.types + t * 12 + 9))));
        std::fprintf(stderr, "\nclubless %zu\n", pick_people(f).clubless.size());
    }
    const auto p = pick_people(f);
    Rng rng{std::mt19937(7)};
    const auto base_params = [&](std::uint32_t person) {
        Params q;
        q.person = person;
        q.type = rng.pick<std::uint32_t>({0, 1, 2, 3, 10, 0x0c, 0x17, 20, 0x1a, 0x1b, 0x1c, 0x2d, 0x30, 0x3b, 0x4d, 0x4e, 0x4f, 0x50, 0x51});
        q.starts_later = rng.pick<std::uint32_t>({0, 0, 1, 0xff});
        q.report = rng.pick<std::uint32_t>({0, 1});
        q.news_argument = rng.below(1000);
        q.chance = rng.pick({0, 0, 5, 100, 127, -5, -30, -128});
        q.pending_only = rng.pick<std::uint32_t>({0, 0, 0, 1, 0x100});
        return q;
    };
    const auto sample_script = [&] { return script(f, random_script(rng, 48)); };
    // The assertion paths and the early returns.
    {
        auto q = base_params(f.s.person(p.healthy_players[0]));
        q.person = 0;
        cases.push_back(registration_case(f, q, {}, "register: no person"));
        q.person = f.s.person(p.healthy_players[0]);
        q.type = 0x52;
        cases.push_back(registration_case(f, q, {}, "register: type 0x52"));
        q.type = 0x1ff;
        cases.push_back(registration_case(f, q, {}, "register: type 0x1ff"));
        q.type = 0x80;
        cases.push_back(registration_case(f, q, {}, "register: type 0x80"));
        q.type = 0xffffffff;
        cases.push_back(registration_case(f, q, {}, "register: type -1"));
        q.type = 3;
        Memory m;
        put(m, 4, q.person + 0x14, 0);
        cases.push_back(registration_case(f, q, m, "register: no profile"));
        q.person = scenario.person(scenario.people());
        Memory beyond;
        put(beyond, 4, q.person, scenario.people());
        cases.push_back(registration_case(f, q, beyond, "register: index past the table"));
        q.person = f.s.person(p.healthy_players[0]);
        q.type = 0x4d;
        Memory index_over;
        put(index_over, 4, q.person, scenario.people() + 7);
        cases.push_back(registration_case(f, q, index_over, "register: person index past the count"));
    }
    // The matrix over people.
    unsigned n = 0;
    const auto add = [&](std::uint32_t index, const char* kind) {
        for (unsigned variant = 0; variant < 8; ++variant) {
            auto q = base_params(f.s.person(index));
            auto memory = tweak_memory(f, q.person, random_tweak(rng));
            append(memory, sample_script());
            auto c = registration_case(f, q, memory, std::string("register: ") + kind + " " + std::to_string(index) +
                                                         " #" + std::to_string(variant) + " type " + std::to_string(q.type));
            c.answers = {{unlink_news, 0}};
            cases.push_back(c);
            ++n;
        }
    };
    for (const auto i : p.healthy_players) add(i, "healthy");
    for (const auto i : p.injured_players) add(i, "injured");
    for (const auto i : p.clubless) add(i, "clubless");
    for (const auto i : p.staff) add(i, "staff");
    // Each type of the old injury allowed to be replaced, and the rest, over the injured.
    for (const auto i : p.injured_players)
        for (const std::uint32_t type : {0x4du, 0x4eu, 0x50u, 0x51u, 4u}) {
            auto q = base_params(f.s.person(i));
            q.type = type;
            q.pending_only = 0;
            q.chance = 0;
            auto memory = sample_script();
            auto c = registration_case(f, q, memory, "register: replaces the injury of " + std::to_string(i) + " with " + std::to_string(type));
            c.answers = {{unlink_news, 0}};
            cases.push_back(c);
        }
    // A pending-only call with a negative chance: the draw of one in -n/10 decides, 0 or not.
    for (const std::uint32_t sample : {0u, 1u, 2u})
        for (const int chance : {-30, -5, -100, 7}) {
            auto q = base_params(f.s.person(p.healthy_players[0]));
            q.type = rng.pick<std::uint32_t>({3, 0x4d, 0x4e});
            q.pending_only = 1;
            q.chance = chance;
            Memory m = script(f, {sample, 5, 5});
            cases.push_back(registration_case(f, q, m, "register: pending only chance " + std::to_string(chance) + " sample " + std::to_string(sample)));
        }
    // A large countdown carried from the replaced injury makes a long one: the alarm rules of the length.
    for (const auto i : p.injured_players)
        for (const std::uint32_t carried : {600u, 3000u, 30000u}) {
            auto q = base_params(f.s.person(i));
            q.type = 0x4e;
            q.pending_only = 0;
            q.chance = rng.pick({0, 5, -30});
            q.starts_later = rng.pick<std::uint32_t>({0, 1});
            Tweak t;
            t.morale = rng.pick<std::int16_t>({0x10, 0x1f, 0x1f, 0x22, 0x25});
            t.prone = rng.pick({0, 11, 40});
            Memory m = tweak_memory(f, q.person, t);
            put(m, 2, f.record_of(i) + 0x34, carried);
            append(m, script(f, random_script(rng, 48)));
            auto c = registration_case(f, q, m, "register: long injury over carried " + std::to_string(carried) + " of " + std::to_string(i));
            c.answers = {{unlink_news, 0}};
            cases.push_back(c);
        }
    // The draws of the sample table at its end: a refill from the generator, answered.
    for (const std::uint32_t value : {0x0u, 0x1234u, 0xfffeu}) {
        auto q = base_params(f.s.person(p.healthy_players[1]));
        q.type = 0x20;
        q.pending_only = 0;
        Memory m;
        put(m, 4, f.cursor_cell, f.table_last);
        Case c = registration_case(f, q, m, "register: refill " + std::to_string(value));
        c.nth_answers = {{redraw, 1, value}};
        cases.push_back(c);
    }
    // The record table without a free record (three records in use), without records, and failing to grow.
    for (unsigned variant = 0; variant < 5; ++variant) {
        auto q = base_params(f.s.person(p.healthy_players[2 + variant]));
        q.type = 3;
        q.pending_only = 0;
        q.chance = 0;
        Memory m = sample_script();
        if (variant == 0 || variant == 3) {
            put(m, 4, f.manager + 8, 3);
            for (std::uint32_t k = 0; k < 3; ++k) put(m, 1, f.records + k * 0x38, 0);
            put(m, 4, f.manager + 8, 3);
        }
        if (variant == 1) put(m, 4, f.manager + 8, 0);
        if (variant == 2 || variant == 4) {
            put(m, 4, f.manager + 4, 0);
            put(m, 4, f.manager + 8, 0);
        }
        Case c = registration_case(f, q, m, "register: record table variant " + std::to_string(variant));
        cases.push_back(c);
    }
    // The match: the person among the first side's players, the second's, or neither.
    {
        constexpr std::uint32_t match = scratch_base + 0x8000, side_a = scratch_base + 0x8100, side_b = scratch_base + 0x8300;
        for (unsigned where = 0; where < 4; ++where) {
            auto q = base_params(f.s.person(p.healthy_players[0]));
            q.type = 5;
            q.pending_only = 0;
            q.match = match;
            Memory m = sample_script();
            put(m, 4, match + 0x1c, side_a);
            put(m, 4, match + 0x20, side_b);
            for (std::uint32_t k = 0; k < 52; ++k) {
                put(m, 4, side_a + 0x98 + k * 4, 0);
                put(m, 4, side_b + 0x98 + k * 4, 0);
            }
            if (where == 0) put(m, 4, side_a + 0x98 + 5 * 4, q.person);
            if (where == 1) put(m, 4, side_b + 0x98 + 49 * 4, q.person);
            if (where == 2) put(m, 4, side_a + 0x98 + 50 * 4, q.person);  // past the 50 searched
            Case c = registration_case(f, q, m, "register: match " + std::to_string(where));
            c.answers = {{copy_match, 0}};
            cases.push_back(c);
        }
    }
    // ---- the outcomes the first matrix did not reach ----
    constexpr std::uint32_t club_check = 0x1014c2e0, loan_check = 0x100d3040;
    const auto healthy = [&](unsigned k) { return f.s.person(p.healthy_players[k % p.healthy_players.size()]); };
    const auto plain = [&](unsigned k, std::uint32_t type) {
        auto q = base_params(healthy(k));
        q.type = type;
        q.pending_only = 0;
        q.chance = 0;
        q.starts_later = 0;
        return q;
    };
    // A person without a club (the club field cleared): no staff skill, and none of the club's tests.
    for (unsigned variant = 0; variant < 6; ++variant) {
        auto q = plain(variant, 3 + variant);
        q.starts_later = variant % 2;
        Memory m = sample_script();
        put(m, 4, q.person + 0x24, 0);
        cases.push_back(registration_case(f, q, m, "register: clubless " + std::to_string(variant)));
    }
    // The club's tests: the club state and the loan test answered true or false; a long injury needs days >= 100.
    for (unsigned variant = 0; variant < 8; ++variant) {
        auto q = plain(variant, 0x4e);
        Tweak t;
        t.morale = 0x10;
        Memory m = tweak_memory(f, q.person, t);
        append(m, sample_script());
        Case c = registration_case(f, q, m, "register: club tests " + std::to_string(variant));
        c.answers = {{club_check, variant & 1}, {loan_check, (variant >> 1) & 1}};
        cases.push_back(c);
    }
    // Long injuries (a large carried countdown) so that the days pass 100 and the club's tests are decisive; the
    // career object absent, or its manager state absent; the press and alert objects absent.
    for (unsigned variant = 0; variant < 12; ++variant) {
        const auto index = p.injured_players[variant % p.injured_players.size()];
        auto q = base_params(f.s.person(index));
        q.type = 0x4e;
        q.pending_only = 0;
        q.chance = 0;
        q.starts_later = 0;
        Tweak t;
        t.morale = 0x10;
        Memory m = tweak_memory(f, q.person, t);
        put(m, 2, f.record_of(index) + 0x34, 3000);
        append(m, sample_script());
        if (variant % 4 == 1) put(m, 4, scenario.toc(-0x7b40), 0);
        if (variant % 4 == 2) put(m, 4, scenario.r32(scenario.toc(-0x7b40)) + 0x988, 0);
        put(m, 4, q.person + 0x20, q.person);  // an id link, so that the press is told
        if (variant % 3 == 1) put(m, 4, scenario.toc(-0x7a64), 0);
        if (variant % 3 == 2) put(m, 4, scenario.toc(-0x7b28), 0);
        Case c = registration_case(f, q, m, "register: objects absent " + std::to_string(variant));
        // With the manager state absent the first test must not pass (it would read the state's table).
        c.answers = {{unlink_news, 0}, {club_check, variant % 4 == 2 ? 1u : 0u}, {loan_check, variant % 5 == 3 ? 1u : 0u}};
        cases.push_back(c);
    }
    // The staff slot holders of the club: no staff member in a slot, or no holder.
    for (unsigned variant = 0; variant < 6; ++variant) {
        auto q = plain(variant, 3);
        const auto club = f.club(q.person);
        Memory m = sample_script();
        const auto holder = f.s.r32(club + 0x190 + 4 * (variant % 3));
        if (variant < 3 && holder) put(m, 4, holder + 0x1c, 0);
        else put(m, 4, club + 0x190 + 4 * (variant % 3), 0);
        cases.push_back(registration_case(f, q, m, "register: staff slot " + std::to_string(variant)));
    }
    // The record table that cannot grow: an empty table of a huge count makes the allocation fail on both sides.
    for (unsigned variant = 0; variant < 2; ++variant) {
        auto q = plain(variant, 3);
        Memory m = sample_script();
        put(m, 4, f.manager + 4, 0);
        put(m, 4, f.manager + 8, variant ? 0x20000000 : 0x10000000);
        cases.push_back(registration_case(f, q, m, "register: record table cannot grow " + std::to_string(variant)));
    }
    // A pending-only call with a negative chance: the draw (random script) decides, zero or not.
    for (unsigned variant = 0; variant < 16; ++variant) {
        auto q = plain(variant, rng.pick<std::uint32_t>({0x0b, 0x11, 0x17, 0x1c, 0x23, 0x29, 0x3b, 0x4c}));
        q.pending_only = 1;
        q.chance = rng.pick({-30, -100, -50, -128});
        cases.push_back(registration_case(f, q, sample_script(), "register: pending only random " + std::to_string(variant)));
    }
    // The same type again while pending: the repeat alert, for every type (the limit of the length is the
    // type's base days, 200 less, kept at 5 at least).
    for (std::uint32_t type = 0; type < 0x52; ++type) {
        auto q = plain(type, type);
        q.chance = rng.pick({0, 0, 5, 20});
        Tweak t;
        t.pending_type = int(type);
        t.pending_count = 1 + int(type % 3);
        t.form = 0x30;
        Memory m = tweak_memory(f, q.person, t);
        append(m, sample_script());
        if (type % 4 == 1) put(m, 4, scenario.toc(-0x7b28), 0);  // no alert object
        Case c = registration_case(f, q, m, "register: repeat of type " + std::to_string(type));
        cases.push_back(c);
    }
    // The alarm of a long injury for a morale between 0x1f and 0x22: only the last rule applies.
    for (const std::int16_t morale : {0x20, 0x21, 0x1f}) {
        for (unsigned variant = 0; variant < 4; ++variant) {
            const auto index = p.injured_players[variant % p.injured_players.size()];
            auto q = base_params(f.s.person(index));
            q.type = 0x4e;
            q.pending_only = 0;
            q.chance = 0;
            Tweak t;
            t.morale = morale;
            t.prone = 0;
            Memory m = tweak_memory(f, q.person, t);
 put(m, 2, f.record_of(index) + 0x34, 30000);
            // Variant 2: every draw 0 (the alarm draw stays under its threshold); variant 3: no alert object.
            append(m, variant == 2 ? script(f, std::vector<std::uint32_t>(600, 0)) : sample_script());
            if (variant == 3) put(m, 4, scenario.toc(-0x7b28), 0);
            Case c = registration_case(f, q, m, "register: alarm morale " + std::to_string(morale) + " " + std::to_string(variant));
            c.answers = {{unlink_news, 0}};
            cases.push_back(c);
        }
    }
    (void)n;
}

INJURY_UNIT_CASES("onset news") {
    if (scenario.match) return;
    const Facts f(scenario);
    const auto p = pick_people(f);
    Rng rng{std::mt19937(11)};
    constexpr std::uint32_t owner_of = 0x10082f20;
    for (const auto index : p.injured_players) {
        const auto person = f.s.person(index);
        const auto record = f.record_of(index);
        for (unsigned variant = 0; variant < 16; ++variant) {
            Memory m;
            // The career object (which lists the people involved: its routine reads the person's links) is
            // absent in half the cases, so that a person without a club or id link can be told too.
            const bool no_career = variant % 2 == 1;
            if (no_career) put(m, 4, scenario.toc(-0x7b40), 0);
            put(m, 2, record + 0x30, rng.pick<std::uint32_t>({20, 100, 101, 400}));
            put(m, 1, record + 0x20, rng.pick<std::uint32_t>({0xff, 0}));
            if (no_career && rng.below(3) == 0) put(m, 4, person + 0x24, 0);
            if (no_career && rng.below(2) == 0) put(m, 4, person + 0x20, 0);
            if (no_career && rng.below(3) == 0) put(m, 4, person + 0x28, 0);
            if (no_career && rng.below(3) == 0) put(m, 4, person + 0x2c, 0);
            if (no_career && rng.below(3) == 0) put(m, 4, person + 0x30, 0);
            put(m, 2, f.profile(person) + 0xc, rng.pick<std::uint32_t>({0, 0x1f40, 0x1f41, 0x2000}));
            if (rng.below(8) == 0) put(m, 4, scenario.toc(-0x7bf0), 0);  // no news object
            put(m, 1, record + 0, rng.pick<std::uint32_t>({0, 1, 20, 0x4d, 0x51, 0xff}));
            Case c{.entry = news, .registers = {{3, f.manager}, {4, person}, {5, record}, {6, rng.below(500)}},
                   .memory = m, .label = "news of " + std::to_string(index) + " #" + std::to_string(variant)};
            if (variant % 5 == 2) c.answers = {{owner_of, 0}};  // the owner's club lookup finds nothing
            cases.push_back(c);
        }
        // The people the career object lists: the count answered (0x103fc710 is outside the unit): none, with the person's id link
        // present or cleared, and the list full (0x1e: the person's own id is not added). A short injury, so that the unfilled list is
        // never walked.
        for (unsigned variant = 0; variant < 3; ++variant) {
            Memory m;
            put(m, 2, record + 0x30, 30);
            put(m, 1, record + 0x20, 0xff);
            put(m, 4, person + 0x20, variant == 1 ? 0u : person);  // an id link (the person's own record: any pointer), or none
            Case c{.entry = news, .registers = {{3, f.manager}, {4, person}, {5, record}, {6, 9}},
                   .memory = m, .label = "news people listed " + std::to_string(variant) + " of " + std::to_string(index)};
            c.answers = {{0x103fc710, variant == 2 ? 0x1eu : 0u}, {0x102cdbf0, 1}};
            cases.push_back(c);
        }
        // A short injury of a person without a club (career object absent): nothing is told.
        {
            Memory m;
            put(m, 4, scenario.toc(-0x7b40), 0);
            put(m, 2, record + 0x30, 30);
            put(m, 1, record + 0x20, 0xff);
            put(m, 4, person + 0x24, 0);
            cases.push_back({.entry = news, .registers = {{3, f.manager}, {4, person}, {5, record}, {6, 3}}, .memory = m,
                             .label = "news short without club of " + std::to_string(index)});
        }
        // A short injury of a person with a club: the club's news takes it (the acceptance answered) and the owner's too;
        // the owner lookup answered 0 or run as the game has it, the person's id link there or not.
        for (unsigned variant = 0; variant < 4; ++variant) {
            Memory m;
            put(m, 4, scenario.toc(-0x7b40), 0);
            put(m, 2, record + 0x30, 30);
            put(m, 1, record + 0x20, 0xff);
            if (variant & 1) put(m, 4, person + 0x20, 0);
            Case c{.entry = news, .registers = {{3, f.manager}, {4, person}, {5, record}, {6, 7}},
                   .memory = m, .label = "news short of " + std::to_string(index) + " #" + std::to_string(variant)};
            c.answers = {{0x102cdbf0, 1}};
            if (variant & 2) c.answers.push_back({owner_of, 0});
            cases.push_back(c);
        }
    }
}

INJURY_UNIT_CASES("onset settle") {
    if (scenario.match) return;
    const Facts f(scenario);
    const auto p = pick_people(f);
    // The first row with a pending injury and a profile: its person is the one the scripts pick.
    std::uint32_t first_pending = 0;
    for (std::uint32_t i = 0; i < scenario.people(); ++i)
        if (f.pending_byte(i) != -1 && f.profile(scenario.person(i))) { first_pending = i; break; }
    // Pending rows before it belong to people without a profile (the loop visits them first and scores them 100): cleared.
    Memory clear_before;
    for (std::uint32_t i = 0; i < first_pending; ++i)
        if (f.pending_byte(i) != -1) put(clear_before, 1, f.row(i) + 0x14, 0xff);
    const auto candidate = scenario.person(first_pending);
    // Its score must reach 95 at once (100 less the announcements and the proneness): both set to 0.
    put(clear_before, 1, f.row(first_pending) + 0x15, 0);
    put(clear_before, 1, f.profile(candidate) + 0x2d, 0);
    const auto club = f.club(candidate);
    // A record whose type can be pending.
    std::uint32_t record = 0, other_record = 0;
    for (const auto i : p.injured_players) {
        const auto r = f.record_of(i);
        const auto type = std::int8_t(f.b(r));
        if (f.type_can_pend(type) && !record) record = r;
        if (!f.type_can_pend(type) && !other_record) other_record = r;
    }
    if (!record) record = f.record_of(p.injured_players[0]);
    if (!other_record) other_record = record;
    const auto target = p.healthy_players[0];
    if (std::getenv("ONSET_FACTS")) std::fprintf(stderr, "settle: target person %x club %x first_pending %u person %x club %x\n", f.s.person(target), f.club(f.s.person(target)), first_pending, candidate, club);
    const std::vector<std::vector<std::uint32_t>> scripts = {
        {0, 100, 0, 5, 7, 9}, {0, 50, 0, 9, 9}, {20, 100, 1, 2}, {1, 100, 0}, {0, 100, 3, 4}, {40, 99, 6, 0},
    };
    unsigned k = 0;
    for (const auto& samples : scripts) {
        for (unsigned variant = 0; variant < 8; ++variant, ++k) {
            Memory m = script(f, samples);
            append(m, clear_before);
            Tweak t;
            t.pending_type = variant == 3 ? 5 : -1;  // the row already pending: nothing happens
            append(m, tweak_memory(f, f.s.person(target), t));
            if (club) {
                put(m, 2, club + 0x1ec, variant % 3 == 0 ? 0x2000 : variant % 3 == 1 ? 0x100 : 0x109b);
                if (variant == 5) put(m, 2, club + 0x1ec, 0x1099);
            }
            if (variant == 6) put(m, 4, candidate + 0x24, 0);
            if (variant == 7) put(m, 4, candidate + 0x14, 0);
            if (variant == 4) put(m, 1, f.row(first_pending) + 0x15, 3);
            const auto rec = variant == 2 ? other_record : record;
            const std::uint32_t classes[] = {0x0b, 0x0c, 0x0d, 0x0e, 0x18, 0x1e, 0x2c, 0x80, 0x0a, 0x4c, 0x30};
            cases.push_back({.entry = settle, .registers = {{3, f.manager}, {4, f.row(target)}, {5, rec}},
                             .memory = m, .answers = {{settle_class, classes[k % 11]}}, .label = "settle script " + std::to_string(k / 8) + " variant " + std::to_string(variant)});
        }
    }
    // The class of the best person (answered: the course block's routine) picks 0x4e, 0x4f or 0x4d through the
    // game's table, or 0x4d past it.
    for (const std::uint32_t klass : {0x0bu, 0x0cu, 0x0du, 0x0eu, 0x0fu, 0x18u, 0x1eu, 0x2cu, 0x0au, 0x4cu, 0x4du, 0x80u, 0xffu}) {
        Memory m = script(f, {0, 100, 0, 5, 7, 9});
        append(m, clear_before);
        Tweak t;
        t.prone = 0;  // the target's row is the first pending one and scores 100
        append(m, tweak_memory(f, f.s.person(target), t));
        if (club) put(m, 2, club + 0x1ec, 0x2000);
        if (const auto own = f.club(f.s.person(target))) put(m, 2, own + 0x1ec, 0x2000);  // the target's row may come first
        cases.push_back({.entry = settle, .registers = {{3, f.manager}, {4, f.row(target)}, {5, record}},
                         .memory = m, .answers = {{settle_class, klass}}, .label = "settle class " + std::to_string(klass)});
    }
    // The best person's club says it is in a state that stops the registration (0x1014c2e0 true): the pending mark is cleared.
    {
        Memory m = script(f, {0, 100, 0, 5, 7, 9});
        append(m, clear_before);
        Tweak t;
        t.prone = 0;
        append(m, tweak_memory(f, f.s.person(target), t));
        if (const auto own = f.club(f.s.person(target))) put(m, 2, own + 0x1ec, 0x2000);
        cases.push_back({.entry = settle, .registers = {{3, f.manager}, {4, f.row(target)}, {5, record}},
                         .memory = m, .answers = {{0x1014c2e0, 1}}, .label = "settle: club state true"});
    }
    // Nobody to look at (no people): no pending row is visited and the pending mark of the row stays.
    {
        Memory m = script(f, {0, 100, 0});
        put(m, 4, scenario.toc(-0x7be4), 0);
        cases.push_back({.entry = settle, .registers = {{3, f.manager}, {4, f.row(target)}, {5, record}},
                         .memory = m, .label = "settle: no people"});
    }
    // The pending-type table row says the type cannot be pending: nothing happens.
    cases.push_back({.entry = settle, .registers = {{3, f.manager}, {4, f.row(target)}, {5, other_record}},
                     .memory = script(f, {0, 100, 0}), .label = "settle: type that cannot be pending"});
}

// The pick of the daily pass's first draw through the scaled draw (a range past 0xffff).
struct Picker {
    double ratio;
    std::int32_t count, tenth;
    explicit Picker(const Scenario& s) : count(std::int32_t(s.people())), tenth(std::int32_t(std::uint32_t(s.people()) * 10u) / 0xffff) {
        const auto floats = s.toc(-0x543c);
        const auto bits = (std::uint64_t(s.r32(floats)) << 32) | s.r32(floats + 4);
        ratio = double(count) / std::bit_cast<double>(bits);
    }
    std::int32_t pick(std::uint32_t first, std::uint32_t second) const {
        const auto remainder = std::int32_t(first % 0xffff);
        const auto whole = std::int32_t(ratio * double(remainder));
        const auto extra = (std::int32_t(second % std::uint32_t(tenth)) + 5) / 10;
        const auto result = whole + extra;
        return result > count - 1 ? count - 1 : result;
    }
};

INJURY_UNIT_CASES("onset daily") {
    if (scenario.match) return;
    const Facts f(scenario);
    Rng rng{std::mt19937(17)};
    const Picker picker(scenario);
    constexpr std::uint32_t league_check = 0x10003dc0;
    // The text routines of the news (they read files the checker has no host for) and the injury history write
    // (0x10748ce0, a file write) are answered; everything else runs as the game has it.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> base_answers = {{news_text_a, 0}, {news_text_b, 0}, {unlink_news, 0}};
    // Reachable people with a profile.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> reachable;  // (person index, first sample)
    {
        std::vector<char> seen(scenario.people(), 0);
        for (std::uint32_t s = 0; s < 0x10000 && reachable.size() < 40; s += 97) {
            const auto index = std::uint32_t(picker.pick(s, 0));
            if (index < scenario.people() && !seen[index] && f.profile(scenario.person(index))) {
                seen[index] = 1;
                reachable.emplace_back(index, s);
            }
        }
    }
    // Dates: the day (halfword +0 of the game date) and year (+2); the season date's first word.
    struct Date { std::uint32_t day, year, season_first; };
    const std::vector<Date> dates = {{354, 2001, 0x016207d1}, {100, 2001, 0x016207d1}, {140, 2001, 0x016207d1},
                                     {170, 2001, 0x016207d1}, {200, 2001, 0x016207d1}, {180, 2001, 0x00b407d1},
                                     {180, 2001, 0x00b507d2}, {180, 2001, 0x007f07d1}, {180, 2001, 0xffb407d1},
                                     {300, 2001, 0x016207d0}};
    unsigned k = 0;
    for (const auto& date : dates) {
        Memory m;
        put(m, 2, f.today, date.day);
        put(m, 2, f.today + 2, date.year);
        put(m, 4, f.season, date.season_first);
        cases.push_back({.entry = daily, .registers = {{3, f.manager}}, .memory = m, .answers = base_answers,
                         .label = "daily: date " + std::to_string(date.day) + "/" + std::to_string(k++)});
    }
    // Open: the notes list at manager+0x28 (destroyed at the start of the pass). 0x10186cd0 builds it with the game's
    // allocator and the destructor frees it; a list built in the scratch area faults in that free on both sides (the
    // allocator takes only its own blocks), and the game's other lists are not shaped like it (a fault at +0x3c).
    // Fewer than 1500 people: the number of rounds stays at its floor of ten, and the picks are plain remainders.
    for (const std::uint32_t people : {700u, 40u, 1500u}) {
        Memory m;
        put(m, 4, scenario.toc(-0x7be4), people);
        cases.push_back({.entry = daily, .registers = {{3, f.manager}}, .memory = m, .answers = base_answers,
                         .label = "daily: " + std::to_string(people) + " people"});
    }
    // The class ranges of the type table: the class whose range holds a type.
    const auto class_of_range = [&](std::int32_t type) {
        for (unsigned c = 0; c < 12; ++c) {
            const auto ranges = scenario.toc(-0x64d0) + c * 3;
            if (type >= std::int8_t(f.b(ranges + 1)) && type <= std::int8_t(f.b(ranges + 2))) return c;
        }
        return 0u;
    };
    // A set-up person picked first; the type 0x10188930 hands back is steered by the class the draw picks (the
    // random branch) or by the pending type (the picker favours it): every body of the original's jump table, the
    // common ending, and a type past the table. `pending` gives the person a pending injury of the target type.
    const std::uint32_t targets[] = {0x0c, 0x17, 0x1a, 0x1b, 0x1f, 0x1c, 0x2d, 0x3b, 0x3c, 0x14, 0x20, 0x40, 0x4d};
    const int fitness_sets[][3] = {{0x14, 0x14, 0x14}, {0x14, 0x11, 0x14}, {0x11, 0x14, 0x12}, {0x10, 0x10, 0x20}, {0x20, 0x20, 0x20}};
    unsigned n = 0;
    for (std::size_t r = 0; r < reachable.size() && r < 6; ++r) {
        const auto [index, first] = reachable[r];
        const auto person = scenario.person(index);
        for (const auto target : targets)
            for (unsigned pending = 0; pending < 3; ++pending)
                for (unsigned fit = 0; fit < 5; ++fit, ++n) {
                    Tweak t;
                    t.level = 0x10;
                    t.form = (fit == 3 && !pending) ? 0x10 : 0x7f;
                    t.prone = 10;
                    t.pending_type = pending == 1 ? int(target) : pending == 2 ? (class_of_range(std::int32_t(target)) == 1 ? 0 : 5) : -1;
                    t.pending_count = pending ? 1 : 0;
                    (void)0;
                    t.fitness_a = fitness_sets[fit][0];
                    t.fitness_b = fitness_sets[fit][1];
                    t.fitness_c = fitness_sets[fit][2];
                    Memory m;
                    put(m, 1, person + 0x62, rng.pick<std::uint32_t>({0xb, 0xf, 0xd}));
                    put(m, 4, f.row(index) + 0x10, 0xffffffff);
                    append(m, tweak_memory(f, person, t));
                    // samples: the pick (two draws), the proneness roll, the one-in-three, then the pending branch's draw
                    // of 1 (nonzero: the pending type wins) and roll 0, or the class and the range draw of 1.
                    const std::uint32_t klass = class_of_range(std::int32_t(target));
                    std::vector<std::uint32_t> samples = {first, 0, 0, 0};
                    if (pending == 2) { for (const auto v : {0u, 0u, klass, 1u}) samples.push_back(v); }  // again 0, roll 0: the random class
                    else if (pending) { samples.push_back(1); samples.push_back(0); }
                    else if (klass != 0) { samples.push_back(klass); samples.push_back(1); }
                    else { samples.push_back(klass); samples.push_back(1); samples.push_back(1); }
                    for (unsigned extra = 0; extra < 14; ++extra) samples.push_back(rng.below(0x10000));
                    append(m, script(f, samples));
                    auto answers = base_answers;
                    answers.emplace_back(league_check, 0);
                    cases.push_back({.entry = daily, .registers = {{3, f.manager}}, .memory = m, .answers = answers,
                                     .label = "daily: person " + std::to_string(index) + " target 0x" + [&] { char b[8]; std::snprintf(b, sizeof b, "%x", target); return std::string(b); }() +
                                              (pending == 1 ? " pending" : pending == 2 ? " pending other class" : " random") + " fitness " + std::to_string(fit)});
                }
    }
    // The league check true ends the person; a form under 0x14 too; the class-7 retry; a level-0x14 profile.
    for (std::size_t r = 0; r < 4 && r < reachable.size(); ++r) {
        const auto [index, first] = reachable[r];
        const auto person = scenario.person(index);
        for (unsigned kind = 0; kind < 4; ++kind) {
            Tweak t;
            t.level = kind == 3 ? 0x14 : 0x0e;
            t.form = kind == 2 ? 0x05 : 0x7f;
            t.prone = 10;
            t.pending_type = -1;
            Memory m;
            put(m, 1, person + 0x62, 0xb);
            put(m, 4, f.row(index) + 0x10, 0xffffffff);
            append(m, tweak_memory(f, person, t));
            std::vector<std::uint32_t> samples = {first, 0};
            if (kind == 3) { samples.push_back(1); samples.push_back(0); }  // level 0x14: the two draws, 1 <= 0 fails -> next? (either)
            samples.insert(samples.end(), {0, 0, 7, 1, 1});
            for (unsigned extra = 0; extra < 14; ++extra) samples.push_back(rng.below(0x10000));
            append(m, script(f, samples));
            auto answers = base_answers;
            answers.emplace_back(league_check, kind == 0 ? 1u : 0u);
            cases.push_back({.entry = daily, .registers = {{3, f.manager}}, .memory = m, .answers = answers,
                             .label = "daily: person " + std::to_string(index) + " special " + std::to_string(kind)});
        }
    }
    // A refill of the sample table in the middle of the pass.
    {
        Memory m;
        put(m, 4, f.cursor_cell, f.table_last - 6);
        cases.push_back({.entry = daily, .registers = {{3, f.manager}}, .memory = m, .answers = base_answers, .label = "daily: refill",
                         .nth_answers = {{redraw, 1, 0x4321}}});
    }
    // No injury records.
    {
        Memory m;
        put(m, 4, f.manager + 8, 0);
        cases.push_back({.entry = daily, .registers = {{3, f.manager}}, .memory = m, .answers = base_answers, .label = "daily: no injury records"});
    }
}
}
}
