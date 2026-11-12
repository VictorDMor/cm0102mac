// DISCIPLINE.CPP upkeep block cases (discipline_checks.hpp): a person moving club (drop, defer, restore), leaving
// (clear) and the ban's news. People, clubs, ruling bodies, bans and pending items are the career snapshot's; a
// case writes (Case::memory, undone afterwards) only the values that move a routine onto a branch the snapshot does
// not take (a body's scope, a club's key source, a slot's body, a pending item's person).
#include "discipline_checks.hpp"
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
constexpr std::uint32_t drop = 0x100df360, defer = 0x100df500, restore = 0x100df6d0, clear = 0x100df9f0, news = 0x100e0700;
constexpr std::uint32_t database_person = 0x10082f20, database_club = 0x10083110;
constexpr std::uint32_t row_size = 0x2c;

// A ban of the snapshot: the person's entry in a ruling body (the body keeps it in its own slot of the row).
struct Held {
    std::uint32_t body_index, body, person, record, entry, row;
    std::uint32_t slot, index;
};
struct Pending { std::uint32_t payload, person, slot, body; };

Write word(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
Write byte(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

struct World {
    const Scenario& s;
    std::uint32_t manager, bodies, rows, records;
    std::vector<Held> held;
    std::vector<Pending> pending;
    explicit World(const Scenario& scenario) : s(scenario) {
        manager = s.global(-0x7aa4);
        bodies = s.r32(manager);
        rows = s.r32(manager + 0x14);
        records = s.r32(manager + 4);
        const auto ban_count = std::int16_t(s.r32(manager + 8));
        for (unsigned body = 0; body < 42; ++body) {
            const auto object = s.r32(bodies + 4 * body);
            const auto entries = s.r32(object + 0xc);
            const auto slot = s.r32(object + 4) >> 24, body_byte = s.r32(object + 4) >> 16 & 0xff;
            for (std::int32_t at = 0; at < std::int32_t(s.r32(object + 0x14)); ++at) {
                const auto entry = entries + std::uint32_t(at) * 0x10;
                const auto person = s.r32(entry);
                const auto index = std::int16_t(s.r32(entry + 4) & 0xffff);
                if (index < 0 || index >= ban_count || person >= s.people() || slot >= 5) continue;
                const auto row = rows + person * row_size;
                if ((s.r32(row + slot * 8) >> 24) != body_byte || s.r32(row + slot * 8 + 4) != std::uint32_t(at)) continue;
                held.push_back({body, object, person, records + std::uint32_t(index) * 0x18, entry, row, slot, std::uint32_t(at)});
            }
        }
        for (auto node = s.r32(manager + 0x20); node; node = s.r32(node + 8)) {
            const auto payload = s.r32(node);
            pending.push_back({payload, s.r32(payload), s.r32(payload + 4) >> 24, s.r32(payload + 8) >> 24});
        }
    }
};

// The club's scope records the routines compare with a body's scope: club key, nation id.
struct Club {
    std::uint32_t record, source, key, nation_record, nation;
};
Club club_of(const Scenario& s, unsigned index) {
    Club c{};
    c.record = s.club(index);
    c.source = s.r32(c.record + 0x2c);
    c.key = s.r32(c.source + 0x68);
    c.nation_record = s.r32(c.source + 0x74);
    c.nation = s.r32(c.nation_record + 8);
    return c;
}

// The writes that make a body's scope cover (or not) the club, and the club's records null where the original tests them.
struct Scope { std::string what; std::vector<Write> writes; };
std::vector<Scope> scopes(std::uint32_t body, const Club& club) {
    const auto key = body + 0x18, nation = body + 0x1c;
    return {
        {"club key covers", {word(key, club.key)}},
        {"club key differs", {word(key, club.key ^ 1)}},
        {"club key, no key source", {word(key, club.key), word(club.record + 0x2c, 0)}},
        {"nation covers", {word(key, 0xffffffffu), word(nation, club.nation)}},
        {"nation differs", {word(key, 0xffffffffu), word(nation, club.nation + 1)}},
        {"no scope", {word(key, 0xffffffffu), word(nation, 0xffffffffu)}},
        {"nation, no key source", {word(key, 0xffffffffu), word(nation, club.nation), word(club.record + 0x2c, 0)}},
        {"nation, no nation record", {word(key, 0xffffffffu), word(nation, club.nation), word(club.source + 0x74, 0)}},
    };
}

DISCIPLINE_CASES("upkeep") {
    if (scenario.match) return;
    const World w(scenario);
    if (w.held.size() < 3 || w.pending.size() < 4) return;
    const auto m = w.manager;
    const auto people = scenario.global(-0x7be0);
    const auto person_at = [&](std::uint32_t id) { return people + id * 0x70; };
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> history_append{{0x10749fc0, 0}};
    // A few real holders of different bodies and kinds of ban.
    std::vector<Held> picks;
    for (const auto& h : w.held) {
        bool fresh = true;
        for (const auto& p : picks) fresh = fresh && (p.body_index != h.body_index || p.person == h.person);
        bool known = false;
        for (const auto& p : picks) known = known || p.person == h.person;
        if (fresh && !known && picks.size() < 4) picks.push_back(h);
    }
    const auto club = club_of(scenario, 5);
    const auto person_word = [&](const Held& h) { return person_at(h.person); };
    // The row writes that move the person's ban to row slot `slot` (the body keeps its ban in the slot it names).
    const auto in_slot = [&](const Held& h, std::uint32_t slot) {
        std::vector<Write> writes;
        if (slot == h.slot) return writes;
        writes = {byte(h.body + 4, slot), byte(h.row + slot * 8, h.body_index), word(h.row + slot * 8 + 4, h.index),
                  byte(h.row + h.slot * 8, 0xff)};
        return writes;
    };
    // The body listed in a row slot that is not its own: the lookup finds nothing (its slot test fails, and the pending
    // list holds no item for the person there).
    const auto misplaced = [&](const Held& h, std::uint32_t slot) {
        return std::vector<Write>{byte(h.row + slot * 8, h.body_index), byte(h.row + h.slot * 8, 0xff)};
    };
    const auto label = [](const char* routine, const std::string& what) { return std::string("upkeep ") + routine + ": " + what; };

    // ---- 0x100df360 drop: every scope outcome with a real ban, every slot, the entry's and the row's states.
    for (const auto& h : picks)
        for (const auto& scope : scopes(h.body, club)) {
            auto memory = scope.writes;
            cases.push_back({.entry = drop, .registers = {{3, m}, {4, person_word(h)}, {6, club.record}}, .memory = memory,
                             .label = label("drop", scope.what + ", body " + std::to_string(h.body_index))});
        }
    {
        const auto& h = picks[0];
        const auto uncovered = scopes(h.body, club)[1].writes;
        const auto with = [&](std::vector<Write> more) { auto all = uncovered; append(all, more); return all; };
        const auto registers = [&](std::uint32_t club_pointer) { return std::vector<std::pair<unsigned, std::uint32_t>>{{3, m}, {4, person_word(h)}, {6, club_pointer}}; };
        for (std::uint32_t slot = 0; slot < 5; ++slot)
            cases.push_back({.entry = drop, .registers = registers(club.record), .memory = with(in_slot(h, slot)),
                             .label = label("drop", "ban in slot " + std::to_string(slot))});
        cases.push_back({.entry = drop, .registers = registers(0), .memory = uncovered, .label = label("drop", "club null, key scope")});
        cases.push_back({.entry = drop, .registers = registers(0), .memory = scopes(h.body, club)[3].writes, .label = label("drop", "club null, nation scope")});
        cases.push_back({.entry = drop, .registers = registers(0), .memory = scopes(h.body, club)[5].writes, .label = label("drop", "club null, no scope")});
        cases.push_back({.entry = drop, .registers = registers(club.record), .memory = with({word(h.entry + 4, (h.index & 0) | 0xffff)}),
                         .label = label("drop", "entry without a ban record")});
        cases.push_back({.entry = drop, .registers = registers(club.record), .memory = with({byte(h.row + 0x28, 1)}),
                         .label = label("drop", "pending count 1 (assertion)")});
        cases.push_back({.entry = drop, .registers = registers(club.record), .memory = with({byte(h.row + 0x28, 0xff)}),
                         .label = label("drop", "pending count -1 (assertion)")});
        // The body has no entry for the person: the lookup returns nothing and the slot stays.
        cases.push_back({.entry = drop, .registers = registers(club.record), .memory = with({word(h.entry, h.person + 1)}),
                         .label = label("drop", "no entry of the person"), });
        cases.push_back({.entry = drop, .registers = registers(club.record), .memory = with(misplaced(h, (h.slot + 1) % 3)),
                         .label = label("drop", "body in a slot that is not its own")});
        // Two slots at once: a second body in another slot of the row.
        if (picks.size() > 1) {
            const auto& g = picks[1];
            auto memory = uncovered;
            append(memory, scopes(g.body, club)[1].writes);
            append(memory, {byte(h.row + 0x10, g.body_index), word(h.row + 0x14, g.index), word(g.entry, h.person), byte(g.body + 4, 2)});
            cases.push_back({.entry = drop, .registers = registers(club.record), .memory = memory, .label = label("drop", "two bodies")});
        }
        // An empty row: nothing to do.
        cases.push_back({.entry = drop, .registers = {{3, m}, {4, person_at(0)}, {6, club.record}}, .memory = {byte(w.rows + 0, 0xff), byte(w.rows + 8, 0xff), byte(w.rows + 0x10, 0xff), byte(w.rows + 0x18, 0xff)},
                         .label = label("drop", "empty row")});
        // A body index of -2 (not -1): read as a body pointer index in front of the array.
    }

    // ---- 0x100df500 defer: moves slots 0-2 to the pending list; slot 3 is left alone.
    for (const auto& h : picks)
        for (const auto& scope : scopes(h.body, club))
            cases.push_back({.entry = defer, .registers = {{3, m}, {4, person_word(h)}}, .memory = scope.writes,
                             .label = label("defer", scope.what + ", body " + std::to_string(h.body_index))});
    {
        const auto& h = picks[0];
        const auto uncovered = scopes(h.body, club)[1].writes;
        const auto with = [&](std::vector<Write> more) { auto all = uncovered; append(all, more); return all; };
        for (std::uint32_t slot = 0; slot < 5; ++slot)
            cases.push_back({.entry = defer, .registers = {{3, m}, {4, person_word(h)}}, .memory = with(in_slot(h, slot)),
                             .label = label("defer", "ban in slot " + std::to_string(slot))});
        cases.push_back({.entry = defer, .registers = {{3, m}, {4, person_word(h)}}, .memory = uncovered, .answers = {{database_club, 0}},
                         .label = label("defer", "no database person (assertion)")});
        cases.push_back({.entry = defer, .registers = {{3, m}, {4, person_word(h)}}, .memory = with({byte(h.row + 0x28, 3)}),
                         .label = label("defer", "pending count 3 before")});
        if (picks.size() > 1) {
            const auto& g = picks[1];
            auto memory = uncovered;
            append(memory, scopes(g.body, club)[1].writes);
            append(memory, {byte(h.row + 0x10, g.body_index), word(h.row + 0x14, g.index), byte(h.row + 0x8, g.body_index), word(h.row + 0xc, g.index)});
            cases.push_back({.entry = defer, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("defer", "three slots")});
        }
        cases.push_back({.entry = defer, .registers = {{3, m}, {4, person_at(0)}}, .memory = {byte(w.rows + 0, 0xff), byte(w.rows + 8, 0xff), byte(w.rows + 0x10, 0xff), byte(w.rows + 0x18, 0xff)},
                         .label = label("defer", "empty row")});
    }

    // ---- 0x100df6d0 restore: a pending item returns to its slot (over a present ban or an empty slot), then the others.
    {
        const auto& h0 = picks[0];
        Held h = h0;
        // The ban sits in row slot 1 for every case here (slot 3 is never restored, slot 4 not at all).
        const auto base = in_slot(h0, 1);
        h.slot = 1;
        const auto push_restore = [&](Case each) {
            each.memory.insert(each.memory.begin(), base.begin(), base.end());
            cases.push_back(std::move(each));
        };
        const auto& item = w.pending[0];
        // Makes the item the person's: its person, slot and body, and the row's count and slot.
        const auto adopt = [&](const Pending& p, const Held& holder, std::uint32_t slot, std::uint32_t body_index, std::uint32_t count) {
            std::vector<Write> writes{word(p.payload, holder.person), byte(p.payload + 4, slot), byte(p.payload + 8, body_index), byte(holder.row + 0x28, count)};
            return writes;
        };
        // 1. items of real people, as the snapshot has them (slot filled or empty, in or out of scope).
        for (std::size_t k : {std::size_t(0), std::size_t(1), w.pending.size() / 2, w.pending.size() - 1})
            cases.push_back({.entry = restore, .registers = {{3, m}, {4, person_at(w.pending[k].person)}}, .label = label("restore", "snapshot item " + std::to_string(k))});
        // 2. the item over a present ban: found, cleared, restored.
        for (std::uint32_t slot : {0u, 1u, 2u, 3u}) {
            auto memory = adopt(item, h, slot, h.body_index, 1);
            append(memory, in_slot(h, slot));
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory,
                             .label = label("restore", "item over a present ban, slot " + std::to_string(slot))});
        }
        {   // over a slot whose body has no entry of the person
            auto memory = adopt(item, h, h.slot, h.body_index, 1);
            append(memory, {word(h.entry, h.person + 1)});
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "item over a body without the person's entry")});
        }
        {   // the entry has no ban record
            auto memory = adopt(item, h, h.slot, h.body_index, 1);
            append(memory, {word(h.entry + 4, 0xffff)});
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "item over an entry without a ban record")});
        }
        {   // over a slot holding a body that is not its own: no entry
            auto memory = adopt(item, h, 2, h.body_index, 1);
            append(memory, misplaced(h, 2));
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "item over a misplaced body")});
            auto second = scopes(h.body, club)[1].writes;
            append(second, misplaced(h, 2));
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = second, .label = label("restore", "second pass, misplaced body")});
        }
        {   // over an empty slot
            auto memory = adopt(item, h, h.slot, h.body_index, 1);
            append(memory, {byte(h.row + h.slot * 8, 0xff)});
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "item over an empty slot")});
        }
        {   // the row's count 0 ends the walk at the first item; a count -1 too
            for (std::uint32_t count : {0u, 0xffu}) {
                auto memory = adopt(item, h, h.slot, h.body_index, count);
                push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory,
                                 .label = label("restore", "row count " + std::to_string(int(std::int8_t(count))) + " stops the walk")});
            }
        }
        {   // the item's slot 4 and an out-of-range slot byte
            for (std::uint32_t slot : {4u, 0xffu}) {
                auto memory = adopt(item, h, slot, h.body_index, 1);
                push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory,
                                 .label = label("restore", "item slot byte " + std::to_string(int(std::int8_t(slot))))});
            }
        }
        // 3. the second pass: every scope outcome, with the item on another slot (so slot h.slot is not restored)
        //    and with no item at all for the person.
        for (const auto& scope : scopes(h.body, club)) {
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = scope.writes,
                             .label = label("restore", "second pass, " + scope.what)});
            auto memory = scope.writes;
            append(memory, adopt(item, h, (h.slot + 1) % 3, h.body_index, 1));
            append(memory, {byte(h.row + ((h.slot + 1) % 3) * 8, 0xff)});
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory,
                             .label = label("restore", "second pass with an item elsewhere, " + scope.what)});
        }
        {   // the second pass: slot 3 is never touched; a slot with no entry stays; entry without a ban record
            const auto uncovered = scopes(h.body, club)[1].writes;
            auto memory = uncovered;
            append(memory, in_slot(h, 3));
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "second pass, slot 3")});
            memory = uncovered;
            append(memory, {word(h.entry, h.person + 1)});
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "second pass, no entry of the person")});
            memory = uncovered;
            append(memory, {word(h.entry + 4, 0xffff)});
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory, .label = label("restore", "second pass, entry without a ban record")});
        }
        // 4. the person's club: none (the lookup answers null), so every body with a present ban is cleared.
        for (const std::size_t which : {std::size_t(0), std::size_t(3), std::size_t(5)})
            push_restore({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = scopes(h.body, club)[which].writes,
                             .answers = {{database_person, 0}}, .label = label("restore", "no club, " + scopes(h.body, club)[which].what)});
    }

    // ---- 0x100df9f0 clear: history appended, bans cleared, pending items dropped, the row reset.
    for (const auto& h : picks)
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .answers = history_append,
                         .label = label("clear", "person with a ban, body " + std::to_string(h.body_index))});
    {
        const auto& h = picks[0];
        for (std::uint32_t slot = 0; slot < 5; ++slot)
            cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = in_slot(h, slot), .answers = history_append,
                             .label = label("clear", "ban in slot " + std::to_string(slot))});
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = misplaced(h, (h.slot + 1) % 3), .answers = history_append,
                         .label = label("clear", "body in a slot that is not its own")});
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = {word(h.entry + 4, 0xffff)}, .answers = history_append,
                         .label = label("clear", "entry without a ban record")});
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = {word(h.entry, h.person + 1)}, .answers = history_append,
                         .label = label("clear", "no entry of the person")});
        for (std::size_t k : {std::size_t(0), std::size_t(1), w.pending.size() / 2, w.pending.size() - 1})
            cases.push_back({.entry = clear, .registers = {{3, m}, {4, w.pending[k].person}}, .answers = history_append,
                             .label = label("clear", "person with pending items, item " + std::to_string(k))});
        {   // pending items and a ban together; counts 1, 2 and -1
            for (std::uint32_t count : {1u, 2u, 0xffu, 0u}) {
                auto memory = std::vector<Write>{word(w.pending[0].payload, h.person), word(w.pending[1].payload, h.person), byte(h.row + 0x28, count)};
                cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = memory, .answers = history_append,
                                 .label = label("clear", "two pending items, count " + std::to_string(int(std::int8_t(count))))});
            }
        }
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = {word(m + 0x20, 0)}, .answers = history_append,
                         .label = label("clear", "empty pending list")});
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, h.person}}, .memory = {word(person_at(h.person) + 0x14, 0)}, .answers = history_append,
                         .label = label("clear", "incomplete history entry (assertion)")});
        cases.push_back({.entry = clear, .registers = {{3, m}, {4, 3}}, .answers = history_append, .label = label("clear", "person without bans")});
    }

    // The person's club in the database lookups (0x10082f20 / 0x10083110 return a record whose word at +4 is the club
    // index), answered: the snapshot's lookup finds nobody. Cases that answer the lookup themselves (null) keep theirs.
    {
        constexpr auto lookup_record = scratch_base + 0x700;
        for (auto& each : cases) {
            if (each.entry != defer && each.entry != restore) continue;
            const auto lookup = each.entry == defer ? database_club : database_person;
            bool answered = false;
            for (const auto& a : each.answers) answered = answered || a.first == lookup;
            if (answered) continue;
            each.answers.push_back({lookup, lookup_record});
            each.memory.push_back(word(lookup_record + 4, 5));
        }
        const auto& h = picks[0];
        for (const auto& scope : scopes(h.body, club))
            {
                auto memory = in_slot(h, 1);
                append(memory, scope.writes);
                cases.push_back({.entry = restore, .registers = {{3, m}, {4, person_word(h)}}, .memory = memory,
                                 .label = label("restore", "snapshot lookup (no club), " + scope.what)});
            }
    }

    // ---- 0x100e0700 news: the recipients, the parameter list, the fixture and the posting.
    {
        const auto& h = picks[0];
        const auto person = person_at(h.person);
        const auto ban = h.record;
        constexpr auto fixture = scratch_base + 0x400, side_a = scratch_base + 0x500, side_b = scratch_base + 0x540;
        const auto today = scenario.toc(-0x7b80);
        const auto recipients = scenario.toc(-0x7b40);
        // The text and news routines run for real in some cases; the ones that need a news screen are answered.
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> quiet{{0x102cc140, 0}, {0x102cc8e0, 0}, {0x102cdbf0, 1}, {0x102cb4b0, 0},
                                                                         {0x102bdf90, 0}, {0x10715880, 0}, {0x1030d6e0, 0}, {0x1030e110, 0},
                                                                         {0x1030d4a0, 0}, {0x1030dcd0, 0}};
        auto answers_with = [&](std::uint32_t entry, std::uint32_t value, bool recipients_too = true) {
            auto list = quiet;
            std::erase_if(list, [&](const auto& answer) { return answer.first == entry; });  // the first answer for an entry wins
            list.push_back({entry, value});
            if (recipients_too && entry != 0x103fc710) list.push_back({0x103fc710, 3});
            return list;
        };
        const auto fixture_writes = [&](std::uint32_t home_side, std::uint32_t away_side, std::uint32_t person_side) {
            return std::vector<Write>{word(fixture + 0x1c, home_side), word(fixture + 0x20, away_side), {2, fixture + 0x2e, 0x1234},
                                      word(person + 0x20, person_side), word(person + 0x24, person_side)};
        };
        const auto ban_writes = [&](int amount, std::uint32_t flags) {
            return std::vector<Write>{{2, ban + 2, std::uint32_t(amount)}, byte(ban + 0x14, flags)};
        };
        const auto date_word = [&](int low, int high) {
            const auto word0 = scenario.r32(ban + 4);
            return word(today, ((((word0 >> 16) + std::uint32_t(high)) & 0xffff) << 16) | (((word0 & 0xffff) + std::uint32_t(low)) & 0xffff));
        };
        const auto add = [&](const std::string& what, std::uint32_t fixture_pointer, std::vector<Write> memory,
                             std::vector<std::pair<std::uint32_t, std::uint32_t>> answers) {
            cases.push_back({.entry = news, .registers = {{4, person}, {5, ban}, {6, fixture_pointer}}, .memory = std::move(memory),
                             .label = label("news", what), .answers = std::move(answers)});
        };
        // Every real ban, without a fixture, as the snapshot has them (the callees answered where they need a screen).
        for (const auto& g : picks) add("snapshot ban, no fixture, body " + std::to_string(g.body_index), 0, {}, quiet);
        // The date's four relations: today before, equal to, after the ban's date, in each half.
        for (int low : {-1, 0, 1})
            for (int high : {-1, 0, 1}) add("date offset " + std::to_string(low) + "/" + std::to_string(high), 0, {date_word(low, high)}, quiet);
        // The amount: up to 0x32 and above; the person's human manager and the news manager's can-post answer.
        for (int amount : {0, 0x32, 0x33, 0x7fff, -1}) {
            const auto answers = answers_with(0x102cdbf0, 1);
            add("amount " + std::to_string(amount), 0, ban_writes(amount, 0xe0), answers);
        }
        for (std::uint32_t flags : {0u, 0x20u, 0x40u, 0x80u, 0xe0u}) add("flags " + std::to_string(flags), 0, ban_writes(2, flags), quiet);
        add("small ban, person is not a human manager", 0, [&] { auto m1 = ban_writes(2, 0xe0); append(m1, {word(person + 0x24, 0)}); return m1; }(), quiet);
        add("small ban, can't post", 0, ban_writes(2, 0xe0), answers_with(0x102cdbf0, 0));
        // Large ban: posted to the recipients the source answers, or to everyone for a high-reputation manager record.
        for (std::uint32_t count : {0u, 1u, 3u, 30u, 0x80u})
            add("large ban, " + std::to_string(count) + " recipients", 0, ban_writes(0x40, 0), [&] {
                auto list = quiet;
                list.push_back({0x103fc710, count});
                return list;
            }());
        add("large ban, manager record above 0x1f40", 0, [&] { auto m1 = ban_writes(0x40, 0); append(m1, {word(person + 0x14, scratch_base + 0x600), {2, scratch_base + 0x60c, 0x1f41}}); return m1; }(), quiet);
        add("large ban, manager record at 0x1f40", 0, [&] { auto m1 = ban_writes(0x40, 0); append(m1, {word(person + 0x14, scratch_base + 0x600), {2, scratch_base + 0x60c, 0x1f40}}); return m1; }(), quiet);
        add("large ban, no manager record", 0, [&] { auto m1 = ban_writes(0x40, 0); append(m1, {word(person + 0x14, 0)}); return m1; }(), quiet);
        add("no recipients source", 0, [&] { auto m1 = ban_writes(0x40, 0); append(m1, {word(recipients, 0)}); return m1; }(), quiet);
        // The fixture: the person's side by the home test, and a side that is neither.
        for (std::uint32_t home : {0u, 1u})
            for (const auto& arrangement : {std::pair{"person on side A", 0u}, {"person on side B", 1u}, {"person on neither side", 2u}}) {
                auto writes = fixture_writes(side_a, side_b, arrangement.second == 0 ? side_a : arrangement.second == 1 ? side_b : side_a + 4);
                auto list = answers_with(0x100c7d00, home);
                add(std::string("fixture, home test ") + std::to_string(home) + ", " + arrangement.first, fixture, writes, list);
            }
        add("fixture, small ban", fixture, [&] { auto m1 = fixture_writes(side_a, side_b, side_a); append(m1, ban_writes(2, 0)); return m1; }(), answers_with(0x100c7d00, 1));
        add("fixture, large ban, to everyone", fixture, [&] { auto m1 = fixture_writes(side_a, side_b, side_b); append(m1, ban_writes(0x40, 0)); append(m1, {word(person + 0x14, scratch_base + 0x600), {2, scratch_base + 0x60c, 0x2000}}); return m1; }(), answers_with(0x100c7d00, 0));
        // All callees run for real where the snapshot allows.
        cases.push_back({.entry = news, .registers = {{4, person}, {5, ban}, {6, 0}}, .label = label("news", "real callees, no fixture")});
        cases.push_back({.entry = news, .registers = {{4, person}, {5, ban}, {6, fixture}}, .memory = fixture_writes(side_a, side_b, side_a),
                         .label = label("news", "real callees, fixture"), .answers = {{0x100c7d00, 1}}});
    }
}
}
