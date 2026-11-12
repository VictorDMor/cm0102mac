// DISCIPLINE.CPP bans block cases (discipline_checks.hpp): bans registered, cleared, served and aged,
// from the career snapshot's real ruling bodies, people and ban records, with Case::memory reaching the
// branches the snapshot does not take.
#include "discipline_checks.hpp"
#include <algorithm>
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t register_ban = 0x100dfb30, clear_ban = 0x100e0090, serve_ban = 0x100e0250,
                        reduce_ban = 0x100e0410, serve_key_holders = 0x100e05c0;
constexpr std::uint32_t allocate = 0x10775130, reallocate = 0x107751b0;
constexpr unsigned body_count = 42;
// The history file's append (0x100e7160's callee): a snapshot has no file system behind it, so both sides answer it.
const std::vector<std::pair<std::uint32_t, std::uint32_t>> history_answers{{0x10748ce0, 0}};

// A ban of the snapshot: the person's entry in a ruling body and the record it points at.
struct Held {
    std::uint32_t body, person, record, entry;
    bool days;  // matches byte -1
    bool four;  // has a fourth body (the sorted index follows it)
};
struct World {
    std::uint32_t manager, bodies, rows, records;
    std::int16_t ban_count, ban_capacity;
    std::vector<Held> held;
    std::vector<std::uint32_t> free_people;  // persons with no ban in any slot
    std::vector<std::uint32_t> free_records;  // records marked free (matches -1 and days -1)
};

World read_world(const Scenario& s) {
    World w{};
    w.manager = s.global(-0x7aa4);
    w.bodies = s.r32(w.manager);
    w.rows = s.r32(w.manager + 0x14);
    w.records = s.r32(w.manager + 4);
    w.ban_capacity = std::int16_t(s.r32(w.manager + 8) >> 16), w.ban_count = std::int16_t(s.r32(w.manager + 8));
    for (std::int16_t at = 0; at < w.ban_count; ++at) {
        const auto record = w.records + std::uint32_t(at) * 0x18;
        if ((s.r32(record) >> 24) == 0xff && std::int16_t(s.r32(record) >> 0) == -1) w.free_records.push_back(record);
    }
    for (unsigned body = 0; body < body_count; ++body) {
        const auto object = s.r32(w.bodies + 4 * body);
        const auto entries = s.r32(object + 0xc);
        const auto slot = s.r32(object + 4) >> 24, body_byte = s.r32(object + 4) >> 16 & 0xff;
        for (std::int32_t at = 0; at < std::int32_t(s.r32(object + 0x14)); ++at) {
            const auto entry = entries + std::uint32_t(at) * 0x10;
            const auto person = s.r32(entry);
            const auto index = std::int16_t(s.r32(entry + 4) & 0xffff);
            if (index < 0 || index >= w.ban_count || person >= s.people()) continue;
            // The person's row must point back at this entry, as the lookup requires.
            const auto row = w.rows + person * 0x2c + slot * 8;
            if ((s.r32(row) >> 24) != body_byte || s.r32(row + 4) != std::uint32_t(at)) continue;
            const auto record = w.records + std::uint32_t(index) * 0x18;
            w.held.push_back({body, person, record, entry, (s.r32(record) >> 24) == 0xff, (s.r32(record + 0xc) & 0xff) != 0xff});
        }
    }
    for (std::uint32_t person = 0; person < s.people() && w.free_people.size() < 2; ++person) {
        bool empty = true;
        for (unsigned slot = 0; slot < 5; ++slot) empty = empty && (s.r32(w.rows + person * 0x2c + slot * 8) >> 24) == 0xff;
        if (empty) w.free_people.push_back(person);
    }
    return w;
}

std::vector<Held> some(const std::vector<Held>& all, bool days, bool four, std::size_t n) {
    std::vector<Held> out;
    for (const auto& h : all) if (h.days == days && h.four == four && out.size() < n) out.push_back(h);
    return out;
}

// The ban struct a caller builds (0x18 bytes in scratch memory), written whole so neither side reads
// anything the case did not choose.
struct Spec {
    std::int8_t matches = 2, served = 0;
    std::int16_t days = 5;
    std::uint16_t date_high = 3, date_low = 4;
    std::int8_t bodies[4] = {-1, -1, -1, -1};
    std::uint32_t flags = 0xffffffff;
};
std::vector<Write> struct_writes(std::uint32_t at, const Spec& p) {
    std::vector<Write> w{{1, at, std::uint8_t(p.matches)}, {1, at + 1, std::uint8_t(p.served)}, {2, at + 2, std::uint16_t(p.days)},
                         {2, at + 4, p.date_high}, {2, at + 6, p.date_low}, {1, at + 8, 7}, {1, at + 0x10, 9}, {2, at + 0x12, 0x1234},
                         {4, at + 0x14, p.flags}};
    for (unsigned k = 0; k < 4; ++k) w.push_back({1, at + 0xc + k, std::uint8_t(p.bodies[k])});
    return w;
}
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

DISCIPLINE_CASES("bans") {
    if (scenario.match) return;
    const auto w = read_world(scenario);
    if (w.held.empty() || w.free_people.empty()) return;
    const auto m = w.manager;
    const auto people = scenario.global(-0x7be0);
    const auto date_at = scenario.toc(-0x7b80);  // today's two words
    const auto index_of = [&](const Held& h) { return std::uint32_t((h.record - w.records) / 0x18); };
    const auto kind = [](const Held& h) { return std::string(h.days ? "days" : "matches") + (h.four ? " with fourth body" : ""); };

    const auto days_four = some(w.held, true, true, 2), matches_four = some(w.held, false, true, 2),
               matches_plain = some(w.held, false, false, 2);
    std::vector<Held> picks;
    for (const auto* list : {&days_four, &matches_four, &matches_plain}) picks.insert(picks.end(), list->begin(), list->end());
    if (days_four.empty() || matches_four.empty() || matches_plain.empty()) return;

    // ---- 0x100e0090 clear_ban: a real ban of every kind, and the sorted index's misses.
    for (const auto& h : picks)
        cases.push_back({.entry = clear_ban, .registers = {{3, m}, {4, h.person}, {5, index_of(h)}}, .label = "bans: clear " + kind(h)});
    {
        const auto& h = days_four[0];
        const auto first = people + h.person * 0x70 + 0xc, second = people + h.person * 0x70 + 0x10;
        std::uint32_t other = 0;  // a person whose key source differs: the first lookup misses
        for (const auto& g : w.held)
            if (g.person != h.person && g.four && scenario.r32(people + g.person * 0x70 + 0xc) != scenario.r32(first)) { other = g.person; break; }
        const auto other_pointer = scenario.r32(people + other * 0x70 + 0xc), own_pointer = scenario.r32(first);
        const std::vector<std::pair<unsigned, std::uint32_t>> registers{{3, m}, {4, h.person}, {5, index_of(h)}};
        cases.push_back({.entry = clear_ban, .registers = registers, .label = "bans: clear first key misses, second record hits",
                         .memory = {{4, first, other_pointer}, {4, second, own_pointer}}});
        cases.push_back({.entry = clear_ban, .registers = registers, .label = "bans: clear first key misses, no second record (assertion)",
                         .memory = {{4, first, other_pointer}, {4, second, 0}}});
        cases.push_back({.entry = clear_ban, .registers = registers, .label = "bans: clear both keys miss (assertion)",
                         .memory = {{4, first, other_pointer}, {4, second, other_pointer}}});
        cases.push_back({.entry = clear_ban, .registers = registers, .label = "bans: clear empty sorted index (assertion)",
                         .memory = {{2, m + 0x12, 0}, {4, second, 0}}});
    }

    // ---- 0x100e0250 serve_ban: today around the record's date (the end of a days ban is date + days;
    // days 0 puts it at the date), with and without counting, and the matches served reaching the limit.
    const auto date_word = [&](const Held& h, int low, int high) {
        const auto word = scenario.r32(h.record + 4);
        return ((((word >> 16) + std::uint32_t(high)) & 0xffff) << 16) | (((word & 0xffff) + std::uint32_t(low)) & 0xffff);
    };
    for (const auto& h : {days_four[0], matches_four[0], matches_plain[0]})
        for (int low : {-1, 0, 1})
            for (int high : {-1, 0, 1})
                for (std::uint32_t counted : {0u, 1u})
                    for (bool reaches : {false, true}) {
                        if (h.days && reaches) continue;
                        std::vector<Write> memory{{4, date_at, date_word(h, low, high)}};
                        if (h.days) memory.push_back({2, h.record + 2, 0});
                        if (reaches) memory.push_back({1, h.record + 1, (scenario.r32(h.record) >> 24) - 1u});
                        cases.push_back({.entry = serve_ban, .registers = {{3, m}, {4, h.person}, {5, h.body}, {6, counted}},
                                         .label = "bans: serve " + kind(h) + (reaches ? " reaching the limit" : ""), .memory = memory,
                                         .answers = history_answers});
                    }
    cases.push_back({.entry = serve_ban, .registers = {{3, m}, {4, w.free_people[0]}, {5, 0}, {6, 1}}, .label = "bans: serve no entry", .answers = history_answers});
    cases.push_back({.entry = serve_ban, .registers = {{3, m}, {4, days_four[0].person}, {5, days_four[0].body}, {6, 1}},
                     .label = "bans: serve entry without a ban", .memory = {{2, days_four[0].entry + 6, 0xffff}},
                     .answers = history_answers});
    cases.push_back({.entry = serve_ban, .registers = {{3, m}, {4, days_four[0].person}, {5, days_four[0].body}, {6, 1}},
                     .label = "bans: serve days ban with real days", .answers = history_answers});

    // ---- 0x100e0410 reduce_ban: (counted, matches by default, days, matches, days selector).
    struct Args { std::uint32_t counted, matches_default, days, matches, selector; };
    const std::uint32_t none = 0xffffffff;
    const auto reduce = [&](const Held& h, const Args& a, std::vector<Write> memory, const std::string& what) {
        cases.push_back({.entry = reduce_ban,
                         .registers = {{3, m}, {4, h.person}, {5, h.body}, {6, a.counted}, {7, a.matches_default}, {8, a.days}, {9, a.matches}, {10, a.selector}},
                         .label = "bans: reduce " + what, .memory = std::move(memory)});
    };
    {
        const auto& d = days_four[0];
        const std::vector<Write> five_days{{2, d.record + 2, 5}};
        reduce(d, {0, 1, 1, none, none}, five_days, "days, selector -1, not counted");
        reduce(d, {1, 1, 1, none, none}, five_days, "days, selector -1, counted, days left");
        reduce(d, {1, 1, 7, none, none}, five_days, "days, selector -1, counted, days run out");
        reduce(d, {1, 1, 5, none, none}, five_days, "days, selector -1, counted, days reach 0");
        reduce(d, {0, 1, 2, none, 3}, five_days, "days, selector 3, days left");
        reduce(d, {0, 1, 9, none, 3}, five_days, "days, selector 3, days run out");
        reduce(d, {0, 1, 5, none, 0}, five_days, "days, selector 0, days reach 0");
        reduce(d, {1, 1, 1, 1, none}, {}, "days ban as recorded");
        for (const auto& h : {matches_four[0], matches_plain[0]}) {
            const auto set = [&](int limit, int served) {
                return std::vector<Write>{{1, h.record, std::uint32_t(limit)}, {1, h.record + 1, std::uint32_t(served)}};
            };
            reduce(h, {0, 1, 1, none, none}, set(5, 2), kind(h) + ", matches -1, not counted");
            reduce(h, {1, 1, 1, none, none}, set(5, 2), kind(h) + ", matches -1, counted, limit above served");
            reduce(h, {1, 3, 1, none, none}, set(5, 2), kind(h) + ", matches -1, counted, limit reaches served");
            reduce(h, {1, 4, 1, none, none}, set(5, 2), kind(h) + ", matches -1, counted, limit below served");
            reduce(h, {1, 0xff, 1, 1, none}, set(5, 2), kind(h) + ", matches 1 leaves matches");
            reduce(h, {1, 0xff, 1, 5, none}, set(5, 2), kind(h) + ", matches 5 reaches 0");
            reduce(h, {1, 0xff, 1, 6, none}, set(5, 2), kind(h) + ", matches 6 runs out");
            reduce(h, {1, 1, 1, 1, none}, {}, kind(h) + " as recorded");
        }
        reduce({.person = w.free_people[0]}, {1, 1, 1, none, none}, {}, "no entry");
        reduce(d, {1, 1, 1, none, none}, {{2, d.entry + 6, 0xffff}}, "entry without a ban");
    }

    // ---- 0x100e05c0 serve_key_holders: every ruling body (whose key the index does not hold), then bodies
    // pointed at the key source of an index entry (the first, the last, and every key shared by several
    // persons), alone and with every ban expired.
    for (unsigned body = 0; body < body_count; ++body)
        cases.push_back({.entry = serve_key_holders, .registers = {{3, m}, {4, scenario.r32(w.bodies + 4 * body)}},
                         .label = "bans: key holders of body " + std::to_string(body), .answers = history_answers});
    {
        const auto sorted = scenario.r32(m + 0xc);
        const auto count = std::int16_t(scenario.r32(m + 0x10));
        const auto key_at = [&](std::int16_t at) { return scenario.r32(sorted + std::uint32_t(at) * 8 + 4); };
        std::vector<std::int16_t> positions{0, std::int16_t(count - 1)};
        for (std::int16_t at = 0; at + 1 < count; ++at)
            if (key_at(at) == key_at(at + 1)) { positions.push_back(at); positions.push_back(std::int16_t(at + 1)); }
        for (auto at : positions) {
            const auto person = scenario.r32(sorted + std::uint32_t(at) * 8);
            const auto source = scenario.r32(people + person * 0x70 + 0xc);
            const auto object = scenario.r32(w.bodies);
            for (bool expired : {false, true}) {
                std::vector<Write> memory{{4, object + 0x2c, source}};
                if (expired) memory.push_back({4, date_at, 0x7fff7fff});
                cases.push_back({.entry = serve_key_holders, .registers = {{3, m}, {4, object}},
                                 .label = "bans: key holders at index position " + std::to_string(at) + (expired ? ", all expired" : ""),
                                 .memory = memory, .answers = history_answers});
            }
        }
    }

    // ---- 0x100dfb30 register_ban.
    // Writes that make every free record of the snapshot taken, for the cases that append a record.
    std::vector<Write> no_free_record;
    for (auto record : w.free_records) no_free_record.push_back({1, record, 0});
    const auto ban_at = scratch_base;
    const auto register_case = [&](const std::string& what, std::uint32_t person, const Spec& spec, std::vector<Write> memory = {},
                                   std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, bool free_taken = true) {
        if (free_taken) append(memory, no_free_record);
        append(memory, struct_writes(ban_at, spec));
        cases.push_back({.entry = register_ban, .registers = {{3, m}, {4, person}, {5, ban_at}}, .label = "bans: register " + what,
                         .memory = std::move(memory), .answers = std::move(answers)});
    };
    const auto spec_for = [](std::initializer_list<int> bodies, int matches = 2) {
        Spec s;
        s.matches = std::int8_t(matches);
        unsigned k = 0;
        for (int b : bodies) s.bodies[k++] = std::int8_t(b);
        return s;
    };
    const auto& mm = matches_plain[0];
    const auto& dd = days_four[0];
    // merged into a held ban
    register_case("merge matches into matches", mm.person, spec_for({int(mm.body)}));
    register_case("merge days into matches (new ban has no matches)", mm.person, spec_for({int(mm.body)}, -1));
    register_case("merge matches into days", dd.person, spec_for({int(dd.body)}));
    register_case("merge days into days", dd.person, spec_for({int(dd.body)}, -1));
    register_case("merge through the second body", mm.person, spec_for({40, int(mm.body)}));
    register_case("held entry index -1 falls through to a new record", mm.person, spec_for({int(mm.body)}), {{2, mm.entry + 6, 0xffff}});
    // a person without bans: a new record
    const auto free_person = w.free_people[0];
    register_case("new record, array and index full", free_person, spec_for({0, 1, 2, 3}));
    register_case("new record, index has room", free_person, spec_for({0, 1, 2, 3}), {{2, m + 0x10, 0x20}});
    register_case("new record, no fourth body", free_person, spec_for({0, 1, -1, -1}));
    register_case("new record, one body at slot 2", free_person, spec_for({-1, -1, 4, -1}));
    register_case("new record, array has room", free_person, spec_for({0, 1, 2, 3}), {{2, m + 0xa, std::uint32_t(w.ban_capacity - 1)}});
    register_case("new record, array has room, no fourth body", free_person, spec_for({0, 1, 2, -1}), {{2, m + 0xa, std::uint32_t(w.ban_capacity - 1)}});
    register_case("reuses a free record", free_person, spec_for({0, 1, 2, 3}), {{1, w.records + 5 * 0x18, 0xff}, {2, w.records + 5 * 0x18 + 2, 0xffff}}, {}, false);
    register_case("reuses the last free record", free_person, spec_for({0, 1, 2, 3}),
                  {{1, w.records + std::uint32_t(w.ban_count - 1) * 0x18, 0xff}, {2, w.records + std::uint32_t(w.ban_count - 1) * 0x18 + 2, 0xffff}}, {}, false);
    register_case("a record with matches -1 but days set is not free", free_person, spec_for({0, 1, 2, 3}),
                  {{1, w.records, 0xff}, {2, w.records + 2, 7}});
    register_case("first allocation", free_person, spec_for({0, 1, 2, 3}), {{4, m + 4, 0}, {2, m + 8, 0}, {2, m + 0xa, 0}, {2, m + 0x10, 0x20}});
    register_case("array at its limit (assertion)", free_person, spec_for({0, 1, 2, 3}),
                  {{4, m + 4, scratch_base + 0x1000}, {2, m + 8, 32000}, {2, m + 0xa, 32000}});
    register_case("array above its limit (assertion)", free_person, spec_for({0, 1, 2, 3}),
                  {{4, m + 4, scratch_base + 0x1000}, {2, m + 8, 32001}, {2, m + 0xa, 32001}});
    register_case("reallocation fails (assertion)", free_person, spec_for({0, 1, 2, 3}), {}, {{reallocate, 0}});
    register_case("first allocation fails (assertion)", free_person, spec_for({0, 1, 2, 3}), {{4, m + 4, 0}, {2, m + 8, 0}, {2, m + 0xa, 0}}, {{allocate, 0}});
    register_case("index reallocation fails (assertion)", free_person, spec_for({0, 1, 2, 3}), {{2, m + 0xa, std::uint32_t(w.ban_capacity - 1)}}, {{reallocate, 0}});
    if (w.free_people.size() > 1) register_case("another person, new record", w.free_people[1], spec_for({0, 5, 6, 7}), {{2, m + 0x10, 0x20}});
}
}
