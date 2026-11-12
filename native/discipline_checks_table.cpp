// DISCIPLINE.CPP table block cases (discipline_checks.hpp). People, clubs, competitions, ruling bodies, the person
// table and the pending list are the career snapshot's; a case writes (Case::memory, undone afterwards) only the
// values that move a routine onto a branch the snapshot does not take, and builds words and rows it must hand over
// by address in the scratch area.
#include "discipline_checks.hpp"
#include <algorithm>
#include <map>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t row_size = 0x2c;

struct PendingItem { std::uint32_t node, payload, person; std::int8_t slot, body; };

struct World {
    const Scenario& scenario;
    std::uint32_t manager, rows, bodies, competitions;
    std::vector<PendingItem> items;
    std::map<int, std::vector<std::uint32_t>> competitions_of_body;  // ruling body index -> competition ids
    std::vector<std::uint32_t> ungoverned, absent;                   // competitions with no body (-1) / no object
    std::map<int, std::vector<std::uint32_t>> holders_of_body;       // body -> persons whose row holds it in the body's own slot
    explicit World(const Scenario& s)
        : scenario(s), manager(s.global(-0x7aa4)), rows(s.r32(manager + 0x14)), bodies(s.r32(manager)), competitions(s.global(-0x7b5c)) {
        for (auto node = s.r32(manager + 0x20); node; node = s.r32(node + 8)) {
            const auto payload = s.r32(node);
            items.push_back({node, payload, s.r32(payload), i8(payload + 4), i8(payload + 8)});
        }
        const auto count = s.global(-0x7b60);
        for (std::uint32_t id = 0; id < count; ++id) {
            const auto object = s.r32(competitions + id * 4);
            if (!object) { absent.push_back(id); continue; }
            const auto body = i8(object + 0x4a);
            if (body == -1) ungoverned.push_back(id);
            else competitions_of_body[body].push_back(id);
        }
        // Each body keeps its bans in one slot (the byte at +4 of its object): the persons whose row holds the body there with a value (not -1).
        for (std::uint32_t person = 0; person < s.people(); ++person)
            for (int body = 0; body < 42; ++body) {
                const auto slot = i8(s.r32(bodies + body * 4) + 4);
                if (slot < 0 || slot >= 5 || i8(rows + person * row_size + slot * 8) != body || s.r32(rows + person * row_size + slot * 8 + 4) == 0xffffffffu) continue;
                if (auto& holders = holders_of_body[body]; holders.size() < 4) holders.push_back(person);
            }
    }
    std::int8_t i8(std::uint32_t at) const { return std::int8_t(scenario.s.memory.read<1>(at)); }
    std::uint32_t row(std::uint32_t person) const { return rows + person * row_size; }
    std::uint32_t body_pointer_at(std::uint32_t index) const { return bodies + index * 4; }
    // A competition governed by body `body` (the n-th of them), or one governed by another body.
    std::uint32_t competition_of(int body, std::size_t n = 0) const {
        const auto found = competitions_of_body.find(body);
        return found == competitions_of_body.end() ? 0xffffffffu : found->second[n % found->second.size()];
    }
    std::uint32_t competition_not_of(int body) const {
        for (const auto& [other, ids] : competitions_of_body) if (other != body && other != 0) return ids.front();
        return 0;
    }
};
Write word(std::uint32_t address, std::uint32_t value) { return {4, address, value}; }
Write byte(std::uint32_t address, std::uint32_t value) { return {1, address, value}; }
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;

void table_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto manager_object = scenario.global(-0x7aa4);
    const auto people = scenario.people();
    for (std::uint32_t person : {0u, 1u, 7u, people / 2, people - 1})
        for (std::uint32_t slot = 0; slot < 5; ++slot) {
            const auto row = scenario.r32(manager_object + 0x14) + person * 0x2c + slot * 8;
            const auto body = std::uint32_t(std::int32_t(std::int8_t(scenario.s.memory.read<1>(row))));
            cases.push_back({.entry = 0x100e7040, .registers = {{3, manager_object}, {4, person}, {5, slot}, {6, body}}, .label = "table lookup hit"});
            cases.push_back({.entry = 0x100e7040, .registers = {{3, manager_object}, {4, person}, {5, slot}, {6, body + 1}}, .label = "table lookup miss"});
            cases.push_back({.entry = 0x100e7080, .registers = {{3, manager_object}, {4, person}, {5, slot}, {6, 3}, {7, 0x12345}}, .label = "table set"});
        }
}

// The pending list: next item of a person and slot, and removal of a person's items.
void pending_cases(const World& w, std::vector<Case>& cases) {
    const auto m = w.manager;
    const auto count_at = [&](std::uint32_t person) { return w.row(person) + 0x28; };
    const auto& items = w.items;
    if (items.size() < 16) return;
    for (std::size_t k : {std::size_t(0), std::size_t(1), std::size_t(13), std::size_t(14), items.size() / 2, items.size() - 1}) {
        const auto& item = items[k];
        // The item itself (first, middle, last of the list), the same person on another slot (the whole list is
        // walked), another person's slot on this person, and a slot byte of -1.
        cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, item.person}, {5, std::uint32_t(item.slot)}}, .label = "table next pending: item"});
        cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, item.person}, {5, std::uint32_t(item.slot + 1)}}, .label = "table next pending: person, other slot"});
        cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, item.person}, {5, 0xffffffffu}}, .label = "table next pending: slot -1"});
        cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, items[(k + 3) % items.size()].person}, {5, std::uint32_t(item.slot)}},
                         .label = "table next pending: slot of another person"});
        // Remove the person's items.
        cases.push_back({.entry = 0x100df950, .registers = {{3, m}, {4, w.scenario.person(item.person)}}, .label = "table remove pending: person's item"});
    }
    // A person the list does not hold: the row's count 0 stops at the first item; a count of 1, 2 or -1 as
    // written walks the list to its end (1, 2) or stops at once (-1).
    const auto stranger = [&] { std::uint32_t p = 5; while (std::any_of(items.begin(), items.end(), [&](const auto& i) { return i.person == p; })) ++p; return p; }();
    cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, stranger}, {5, 0}}, .label = "table next pending: row count 0"});
    cases.push_back({.entry = 0x100df950, .registers = {{3, m}, {4, w.scenario.person(stranger)}}, .label = "table remove pending: row count 0"});
    for (std::uint32_t count : {1u, 2u, 0xffu}) {
        cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, stranger}, {5, 0}}, .memory = {byte(count_at(stranger), count)},
                         .label = "table next pending: row count " + std::to_string(int(std::int8_t(count))) + ", no item"});
        cases.push_back({.entry = 0x100df950, .registers = {{3, m}, {4, w.scenario.person(stranger)}}, .memory = {byte(count_at(stranger), count)},
                         .label = "table remove pending: row count " + std::to_string(int(std::int8_t(count))) + ", no item"});
    }
    // Two items of one person: a row count of 1 stops after the first removal, 2 removes both.
    const auto& first = items[3];
    const auto& second = items[4];
    for (std::uint32_t count : {1u, 2u})
        cases.push_back({.entry = 0x100df950, .registers = {{3, m}, {4, w.scenario.person(first.person)}},
                         .memory = {word(second.payload, first.person), byte(count_at(first.person), count)},
                         .label = "table remove pending: two items, row count " + std::to_string(count)});
    // The next item after a first item of the same person but another slot: the walk goes on.
    cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, first.person}, {5, std::uint32_t(second.slot)}},
                     .memory = {word(second.payload, first.person)}, .label = "table next pending: second item of the person"});
    // An empty list (the head null: the cursor starts null).
    cases.push_back({.entry = 0x100e70b0, .registers = {{3, m}, {4, items[0].person}, {5, std::uint32_t(items[0].slot)}}, .memory = {word(m + 0x20, 0)},
                     .label = "table next pending: empty list"});
    cases.push_back({.entry = 0x100df950, .registers = {{3, m}, {4, w.scenario.person(items[0].person)}}, .memory = {word(m + 0x20, 0)},
                     .label = "table remove pending: empty list"});
}

// The history file: position, read, append.
void history_cases(const World& w, std::vector<Case>& cases) {
    const auto m = w.manager;
    const auto people = w.scenario.people();
    const std::vector<std::uint32_t> persons = {0, 1, 5, 13, 82034, people - 1};
    for (auto person : persons) {
        for (std::uint32_t argument : {0u, 7u, 0x12345u})
            cases.push_back({.entry = 0x100e7160, .registers = {{3, m}, {4, person}, {5, argument}}, .label = "table history position", .answers = {{0x10748ce0, 0}}});
        cases.push_back({.entry = 0x100e71a0, .registers = {{3, m}, {4, person}}, .label = "table history read: person in the table", .answers = {{0x10749fd0, 0x1234}}});
        // The record's id not below the people count (the id as written; a negative id is below).
        cases.push_back({.entry = 0x100e71a0, .registers = {{3, m}, {4, person}}, .memory = {word(w.scenario.person(person), people)},
                         .label = "table history read: id at the count", .answers = {{0x10749fd0, 0x1234}}});
        cases.push_back({.entry = 0x100e71a0, .registers = {{3, m}, {4, person}}, .memory = {word(w.scenario.person(person), 0x80000000u)},
                         .label = "table history read: negative id", .answers = {{0x10749fd0, 0x1234}}});
        // The entry (scratch): id and a nonzero word at +0x14; the history file's append is answered (no disk).
        constexpr auto entry = scratch_base + 0x200;
        for (std::uint32_t nonzero : {1u, 0x12345u})
            cases.push_back({.entry = 0x100e8660, .registers = {{3, m}, {4, entry}}, .memory = {word(entry, person), word(entry + 0x14, nonzero)},
                             .label = "table history append", .answers = {{0x10749fc0, 0}}});
    }
    // The record pointer null: the people table pointer 0 and person 0.
    cases.push_back({.entry = 0x100e71a0, .registers = {{3, m}, {4, 0}}, .memory = {word(w.scenario.toc(-0x7be0), 0)}, .label = "table history read: null record", .answers = {{0x10749fd0, 0x1234}}});
    constexpr auto entry = scratch_base + 0x200;
    cases.push_back({.entry = 0x100e8660, .registers = {{3, m}, {4, 0}}, .label = "table history append: null entry", .answers = {{0x10749fc0, 0}}});
    cases.push_back({.entry = 0x100e8660, .registers = {{3, m}, {4, entry}}, .memory = {word(entry, 5), word(entry + 0x14, 0)},
                     .label = "table history append: incomplete entry", .answers = {{0x10749fc0, 0}}});
}

// The pending sort's comparator on 8-byte items (person word, key word) in the scratch area.
void comparator_cases(const World& w, std::vector<Case>& cases) {
    constexpr auto a = scratch_base + 0x300, b = scratch_base + 0x310;
    struct Pair { std::uint32_t person_a, key_a, person_b, key_b; const char* label; };
    const auto p = [&](std::size_t k) { return w.items[k % w.items.size()].person; };
    const auto v = [&](std::size_t k) { return w.scenario.r32(w.items[k % w.items.size()].payload + 0xc); };
    const Pair pairs[] = {
        {p(0), v(0), p(1), v(1), "keys differ, first below"},
        {p(1), v(1), p(0), v(0), "keys differ, first above"},
        {p(0), 0x7fffffffu, p(1), 0xffffffffu, "keys differ, difference overflows"},
        {p(0), 0x80000000u, p(1), 5, "keys differ, difference wraps"},
        {p(0), 7, p(1), 7, "same key, first person below"},
        {p(1), 7, p(0), 7, "same key, first person above"},
        {p(2), 7, p(2), 7, "same key, same person"},
        {0xffffffffu, 7, p(1), 7, "same key, first person wildcard"},
        {p(0), 7, 0xffffffffu, 7, "same key, second person wildcard"},
        {0xffffffffu, 7, 0xffffffffu, 7, "same key, both wildcard"},
    };
    for (const auto& pair : pairs)
        cases.push_back({.entry = 0x100e9290, .registers = {{3, a}, {4, b}},
                         .memory = {word(a, pair.person_a), word(a + 4, pair.key_a), word(b, pair.person_b), word(b + 4, pair.key_b)},
                         .label = std::string("table comparator: ") + pair.label});
}

void allocation_cases(const World& w, std::vector<Case>& cases) {
    const auto m = w.manager;
    const auto count_at = w.scenario.toc(-0x7be4);
    cases.push_back({.entry = 0x100e7bd0, .registers = {{3, m}}, .label = "table allocate: the snapshot's people"});
    for (std::uint32_t count : {0u, 1u, 5u, 0x80000000u})
        cases.push_back({.entry = 0x100e7bd0, .registers = {{3, m}}, .memory = {word(count_at, count)}, .label = "table allocate: count " + std::to_string(count)});
    cases.push_back({.entry = 0x100e7bd0, .registers = {{3, m}}, .memory = {word(count_at, 5)}, .label = "table allocate: allocation fails",
                     .answers = {{0x10775130, 0}}});
}

// The ruling bodies' virtual methods.
void notify_cases(const World& w, std::vector<Case>& cases) {
    const auto m = w.manager;
    constexpr auto id_word = scratch_base + 0x40;
    const std::uint32_t governed[] = {w.competition_of(4), w.competition_of(12), w.competition_of(26), w.competition_of(41), w.competition_of(2)};
    for (auto id : governed)
        cases.push_back({.entry = 0x100df260, .registers = {{3, m}, {4, id_word}}, .memory = {word(id_word, id)}, .label = "table notify competition: governed"});
    cases.push_back({.entry = 0x100df260, .registers = {{3, m}, {4, id_word}}, .memory = {word(id_word, w.ungoverned.front())}, .label = "table notify competition: no body (-1)"});
    cases.push_back({.entry = 0x100df260, .registers = {{3, m}, {4, id_word}}, .memory = {word(id_word, w.absent.front())}, .label = "table notify competition: no object"});
    cases.push_back({.entry = 0x100df260, .registers = {{3, m}, {4, id_word}},
                     .memory = {word(id_word, w.competition_of(4)), word(w.body_pointer_at(4), 0)}, .label = "table notify competition: body slot empty"});
    for (std::uint32_t argument : {0u, 1u, 0x1234u})
        cases.push_back({.entry = 0x100df2d0, .registers = {{3, m}, {4, argument}}, .label = "table notify all"});
    cases.push_back({.entry = 0x100df2d0, .registers = {{3, m}, {4, 1}}, .memory = {word(w.body_pointer_at(5), 0), word(w.body_pointer_at(41), 0)},
                     .label = "table notify all: two body slots empty"});
}

// The suspension queries.
void query_cases(const World& w, std::vector<Case>& cases) {
    const auto m = w.manager;
    const auto people = w.scenario.people();
    constexpr auto id_word = scratch_base + 0x40, human = scratch_base + 0x400;
    const Registers none{};
    struct Ask { std::uint32_t person; std::uint32_t competition; const char* label; };
    std::vector<Ask> asks;
    // Persons with a pending ban, against a competition their ruling body governs and one it does not.
    for (std::size_t k = 0; k < w.items.size(); k += 3) {
        const auto& item = w.items[k];
        asks.push_back({item.person, w.competition_of(item.body, k), "pending ban, competition of its body"});
        asks.push_back({item.person, w.competition_not_of(item.body), "pending ban, other competition"});
    }
    // Persons whose row holds the body in the body's own slot, against its competitions.
    for (const auto& [body, persons] : w.holders_of_body)
        for (auto person : persons)
            asks.push_back({person, w.competition_of(body, person), "slot holder, competition of its body"});
    // Persons whose row holds slots, sampled across the table.
    unsigned sampled = 0;
    for (std::uint32_t person = 0; person < people && sampled < 24; person += 4099) {
        bool occupied = false;
        for (int slot = 0; slot < 5; ++slot) occupied |= w.i8(w.row(person) + slot * 8) != -1;
        if (!occupied) continue;
        ++sampled;
        for (int slot = 0; slot < 5; ++slot)
            if (const auto body = w.i8(w.row(person) + slot * 8); body != -1)
                asks.push_back({person, w.competition_of(body, person), "row slot, competition of its body"});
    }
    for (const auto& ask : asks)
        for (const Registers& extra : {Registers{{6, 0}, {7, 0}}, Registers{{6, 5}, {7, 0}}}) {
            Registers registers = {{3, m}, {4, w.scenario.person(ask.person)}, {5, id_word}};
            registers.insert(registers.end(), extra.begin(), extra.end());
            const auto id = ask.competition;
            cases.push_back({.entry = 0x100e7920, .registers = registers, .memory = {word(id_word, id)}, .label = std::string("table suspended: ") + ask.label});
        }
    // Branches the snapshot does not take, on one person with a ban.
    const auto& item = w.items.front();
    const auto person = w.scenario.person(item.person);
    const auto id = w.competition_of(item.body);
    const Registers base{{3, m}, {4, person}, {5, id_word}, {6, 0}, {7, 0}};
    const auto suspended = [&](std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, const char* label) {
        memory.push_back(word(id_word, id));
        cases.push_back({.entry = 0x100e7920, .registers = base, .memory = std::move(memory), .label = std::string("table suspended: ") + label,
                         .answers = std::move(answers)});
    };
    suspended({word(person, people)}, {}, "person id not below the count");
    suspended({word(person, 0x80000000u)}, {}, "negative person id");
    suspended({word(w.scenario.toc(-0x7b5c), 0), }, {}, "no competition table");
    cases.push_back({.entry = 0x100e7920, .registers = base, .memory = {word(id_word, w.absent.front())}, .label = "table suspended: no competition object"});
    cases.push_back({.entry = 0x100e7920, .registers = base, .memory = {word(id_word, w.ungoverned.front())}, .label = "table suspended: competition without body"});
    suspended({word(w.body_pointer_at(item.body), 0)}, {}, "body slot empty");
    // The ruling bodies' answers are given (the same for each ask): the general body bans, or nobody.
    suspended({}, {{0x105bc600, 1}}, "general body bans");
    suspended({}, {{0x105bc600, 0}}, "no ban");
    // A ban that stands for the second ask: a person holding a body in the body's slot, with the ban record's dates
    // rewritten so the original's date test passes (the record's +6 halfword 0, its +2 halfword not -1). Then the
    // person's human manager (an object the case builds in the scratch area): none; one the check does not accept
    // (answered 0); one it accepts (answered 1) with the person in its list of 20 (first, middle, last) or absent.
    std::uint32_t holder = 0, holder_competition = 0;
    std::vector<Write> standing;
    for (const auto& [body, persons] : w.holders_of_body) {
        if (body == 0 || w.competition_of(body) == 0xffffffffu || holder) continue;
        const auto object = w.scenario.r32(w.bodies + body * 4);
        for (auto candidate : persons) {
            const auto value = w.scenario.r32(w.row(candidate) + std::uint32_t(w.i8(object + 4)) * 8 + 4);
            const auto index = std::int16_t(w.scenario.s.memory.read<2>(w.scenario.r32(object + 0xc) + value * 16 + 6));
            if (index == -1) continue;
            const auto record = w.scenario.r32(m + 4) + std::uint32_t(std::int32_t(index)) * 0x18;
            holder = candidate;
            holder_competition = w.competition_of(body);
            standing = {{2, record + 6, 0}, {2, record + 2, 0}};
            break;
        }
    }
    if (holder) {
        const auto holder_person = w.scenario.person(holder);
        const auto banned = [&](std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, const char* label) {
            memory.insert(memory.end(), standing.begin(), standing.end());
            memory.push_back(word(id_word, holder_competition));
            cases.push_back({.entry = 0x100e7920, .registers = {{3, m}, {4, holder_person}, {5, id_word}, {6, 0}, {7, 0}}, .memory = std::move(memory),
                             .label = std::string("table suspended: ban stands, ") + label, .answers = std::move(answers)});
        };
        banned({word(holder_person + 0x24, 0)}, {}, "no human manager");
        for (std::uint32_t slot : {0u, 7u, 19u, 20u}) {
            std::vector<Write> memory = {word(holder_person + 0x24, human)};
            for (std::uint32_t n = 0; n < 20; ++n) memory.push_back(word(human + 0x19c + n * 4, n == slot ? holder_person : 0x1000 + n));
            banned(memory, {{0x1014c2e0, 1}}, slot == 20 ? "human manager accepts, person not in its list" : "human manager accepts, person in its list");
            if (slot == 0) banned(memory, {{0x1014c2e0, 0}}, "human manager does not accept");
        }
    }
    // The governing body's answers for a person, by competition and by slot.
    for (const auto& ask : asks)
        cases.push_back({.entry = 0x100e7a90, .registers = {{3, m}, {4, w.scenario.person(ask.person)}, {5, id_word}},
                         .memory = {word(id_word, ask.competition)}, .label = std::string("table body query: ") + ask.label});
    const Registers query{{3, m}, {4, person}, {5, id_word}};
    const auto queried = [&](std::vector<Write> memory, std::uint32_t competition, Registers registers, const char* label) {
        memory.push_back(word(id_word, competition));
        cases.push_back({.entry = 0x100e7a90, .registers = std::move(registers), .memory = std::move(memory), .label = std::string("table body query: ") + label});
    };
    queried({word(person, people)}, id, query, "person id not below the count");
    queried({word(person, 0x80000000u)}, id, query, "negative person id");
    queried({}, id, {{3, m}, {4, 0}, {5, id_word}}, "no person");
    queried({}, w.absent.front(), query, "no competition object");
    queried({}, w.ungoverned.front(), query, "competition without body");
    queried({word(w.body_pointer_at(item.body), 0)}, id, query, "body slot empty");
    unsigned slot_persons = 0;
    for (std::uint32_t candidate = 0; candidate < people && slot_persons < 20; candidate += 3571) {
        bool occupied = false;
        for (int slot = 0; slot < 5; ++slot) occupied |= w.i8(w.row(candidate) + slot * 8) != -1;
        if (!occupied && slot_persons % 4 != 3) continue;  // some persons with empty rows too
        ++slot_persons;
        for (std::uint32_t slot : {0u, 1u, 2u, 3u, 4u, 0xffffffffu})
            for (const Registers& extra : {Registers{{6, 0}, {7, 0}}, Registers{{6, 3}, {7, 9}}}) {
                Registers registers = {{3, m}, {4, w.scenario.person(candidate)}, {5, slot}};
                registers.insert(registers.end(), extra.begin(), extra.end());
                cases.push_back({.entry = 0x100e7b40, .registers = registers, .label = "table slot query: row slot"});
            }
    }
    const Registers slot_query{{3, m}, {4, person}, {5, std::uint32_t(item.slot)}, {6, 0}, {7, 0}};
    cases.push_back({.entry = 0x100e7b40, .registers = slot_query, .label = "table slot query: pending ban's slot"});
    cases.push_back({.entry = 0x100e7b40, .registers = slot_query, .memory = {word(person, people)}, .label = "table slot query: person id not below the count"});
    cases.push_back({.entry = 0x100e7b40, .registers = slot_query, .memory = {word(person, 0x80000000u)}, .label = "table slot query: negative person id"});
}

// The match-day hook: a fixture (scratch) of a competition, with two sides. The ruling bodies' own fixture routine
// (0x105bbee0) is answered: what is checked is which bodies are asked, in which order and with which flags.
void fixture_cases(const World& w, std::vector<Case>& cases) {
    const auto m = w.manager;
    constexpr auto fixture = scratch_base + 0x500, scratch_side = scratch_base + 0x700, scratch_league = scratch_base + 0x780;
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    // Two real clubs whose leagues have different bodies.
    const auto league_body = [&](std::uint32_t club) -> int {  // the ruling body of the club's league, -1 if none
        const auto league = w.scenario.r32(club + 0x30);
        if (!league || w.scenario.r32(league) >= w.scenario.global(-0x7b60)) return -1;
        const auto object = w.scenario.r32(w.competitions + w.scenario.r32(league) * 4);
        return object ? w.i8(object + 0x4a) : -1;
    };
    std::uint32_t home = 0, away = 0;
    for (unsigned club = 0; club < 5000 && !away; ++club) {
        const auto candidate = w.scenario.club(club);
        if (league_body(candidate) < 1) continue;
        if (!home) home = candidate;
        else if (league_body(candidate) != league_body(home)) away = candidate;
    }
    if (!home || !away) return;
    const auto friendly = w.scenario.global(-0x7aa8);
    const auto home_body = league_body(home), away_body = league_body(away);
    // A side the case builds: a record whose +0x30 points at a word holding the league's competition id.
    const auto made_side = [&](std::uint32_t index, std::uint32_t league_competition) {
        const auto side = scratch_side + index * 0x40, league = scratch_league + index * 8;
        return std::pair{side, std::vector<Write>{word(side + 0x30, league), word(league, league_competition)}};
    };
    const auto add = [&](std::uint32_t competition, std::uint32_t home_side, std::uint32_t away_side, std::vector<Write> extra, Answers answers,
                         const std::string& label) {
        std::vector<Write> memory = {word(fixture, competition), word(fixture + 0x14, 0x1234), word(fixture + 0x1c, home_side), word(fixture + 0x20, away_side)};
        memory.insert(memory.end(), extra.begin(), extra.end());
        answers.push_back({0x105bbee0, 0});
        cases.push_back({.entry = 0x100e6e90, .registers = {{3, m}, {4, fixture}, {5, 0x77}}, .memory = std::move(memory),
                         .label = "table match day: " + label, .answers = std::move(answers)});
    };
    const auto filter = [](std::uint32_t accepts) { return Answers{{0x10494d20, accepts}}; };
    const auto governed = w.competition_of(4), ungoverned = w.ungoverned.front();
    // The fixture's own competition has a body.
    add(governed, home, away, {}, filter(1), "competition with a body");
    // None (unset, absent, ungoverned, its body's slot empty): the filter decides, answered both ways; a rejected
    // fixture of the friendly competition goes on, any other stops.
    const std::pair<std::uint32_t, std::vector<Write>> no_body[] = {
        {0xffffffffu, {}}, {w.absent.front(), {}}, {ungoverned, {}}, {governed, {word(w.body_pointer_at(4), 0)}}};
    for (const auto& [competition, extra] : no_body)
        for (std::uint32_t accepts : {0u, 1u}) add(competition, home, away, extra, filter(accepts), std::string("no body, filter ") + (accepts ? "accepts" : "rejects"));
    add(friendly, home, away, {}, filter(0), "friendly competition, filter rejects");
    add(friendly, home, away, {}, filter(1), "friendly competition, filter accepts");
    // The sides' leagues: both have bodies; one side has none of the kinds below.
    const auto [null_league_side, null_league_memory] = made_side(0, 0);
    const auto [absent_side, absent_memory] = made_side(1, w.absent.front());
    const auto [ungoverned_side, ungoverned_memory] = made_side(2, ungoverned);
    std::vector<Write> null_pointer = null_league_memory;
    null_pointer[0] = word(null_league_side + 0x30, 0);
    const auto with = [](std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    add(ungoverned, home, away, {}, filter(1), "both sides' bodies");
    add(ungoverned, null_league_side, away, null_pointer, filter(1), "home league pointer null");
    add(ungoverned, home, null_league_side, null_pointer, filter(1), "away league pointer null");
    add(ungoverned, absent_side, away, absent_memory, filter(1), "home league without object");
    add(ungoverned, home, absent_side, absent_memory, filter(1), "away league without object");
    add(ungoverned, ungoverned_side, away, ungoverned_memory, filter(1), "home league without body");
    add(ungoverned, home, ungoverned_side, ungoverned_memory, filter(1), "away league without body");
    add(ungoverned, home, away, {word(w.body_pointer_at(std::uint32_t(home_body)), 0)}, filter(1), "home body slot empty");
    add(ungoverned, home, away, {word(w.body_pointer_at(std::uint32_t(away_body)), 0)}, filter(1), "away body slot empty");
    add(ungoverned, absent_side, ungoverned_side, with(absent_memory, ungoverned_memory), filter(1), "neither side has a body");
}

DISCIPLINE_CASES("table") {
    if (scenario.match) return;
    if (!scenario.global(-0x7aa4)) return;
    const World w(scenario);
    table_cases(scenario, cases);
    pending_cases(w, cases);
    history_cases(w, cases);
    comparator_cases(w, cases);
    allocation_cases(w, cases);
    notify_cases(w, cases);
    query_cases(w, cases);
    fixture_cases(w, cases);
}
}
