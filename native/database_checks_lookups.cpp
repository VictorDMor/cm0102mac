// cm-database-checks fixtures for DATABASE.CPP's lookups block (database_checks.hpp).
// Every routine is driven through its branches by a world of records and globals:
// each TOC slot points at a word holding a distinct value (global_value), so a
// record field set to one of them hits exactly that link of a chain; enumerations
// cover each exit, and seeded random mixes of the choices cover the combinations
// (the long gate routines). External callees (the contract manager, the date, the
// assertion, 0x101929b0, 0x1014c2e0) are scripted and record their arguments.
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;

constexpr std::uint32_t cells = data + 0x1000;
constexpr std::uint32_t record_at(unsigned n) { return data + 0x4000 + n * 0x400; }
constexpr std::uint32_t rows = data + 0x10200, link_records = data + 0x11000, clubs = data + 0x12000,
                        nations = data + 0x14000, pool = data + 0x15000, date_words = data + 0x16000;
constexpr std::uint32_t zero_from = data + 0x4000, zero_to = data + 0x17000;

std::vector<std::int32_t> slot_table() {
    std::set<std::int32_t> slots{
        -0x7a10, -0x7a00, -0x7af4, -0x7a14, -0x7a18, -0x7a28, -0x7628, -0x762c, -0x7630, -0x7634, -0x7638,
        -0x763c, -0x7640, -0x7644, -0x7648, -0x764c, -0x7508, -0x7590, -0x7b68, -0x75a8, -0x75b0, -0x758c,
        -0x7a70, -0x7a68, -0x7a1c, -0x7574, -0x757c, -0x7578, -0x74f4, -0x7520, -0x750c, -0x75c4, -0x7504,
        -0x75c8, -0x7654, -0x7650, -0x7658, -0x79fc, -0x7ab0, -0x7b9c, -0x7be4, -0x7be0, -0x7be8, -0x7b7c,
        -0x7b78};
    for (int i = 0; i < 23; ++i) slots.insert(-0x75cc - 4 * i);
    return {slots.begin(), slots.end()};
}
const std::vector<std::int32_t> all_slots = slot_table();
unsigned slot_index(std::int32_t slot) {
    return unsigned(std::find(all_slots.begin(), all_slots.end(), slot) - all_slots.begin());
}
std::uint32_t cell(std::int32_t slot) { return cells + 4 * slot_index(slot); }
// The word a nation/id slot points at: distinct per slot.
// Odd slots hold words with the high bit set, so signed and unsigned compares of a record's
// field with different slots' words disagree on cr0.
std::uint32_t global_value(std::int32_t slot) {
    const auto index = slot_index(slot);
    return (0x1000 + 0x11 * index) | (index & 1 ? 0x80000000u : 0u);
}
constexpr std::uint32_t miss = 0x7777, high_miss = 0x80007777;

// What the scripted callees answer.
struct Script {
    std::uint32_t contract = 0;       // 0x10082f20 (primary contract)
    std::uint32_t club_contract = 0;  // 0x10082f90
    std::uint32_t club_answer = 0;    // 0x1014c2e0
    bool clobber = false;
};
std::map<std::uint32_t, std::uint32_t> callee_results(const Script& t) {
    return {{0x10082f20, t.contract}, {0x10082f90, t.club_contract}, {0x1014c2e0, t.club_answer},
            {0x107156f0, date_words}, {0x101929b0, 0x4242}, {0x107755c0, 0}};
}
bool scripted(State& s, const Script& t) {
    const auto results = callee_results(t);
    const auto found = results.find(s.pc);
    if (found == results.end()) return false;
    Cpu c(s);
    const auto calls = c.u32(trace_at);
    record_call(c);
    if (t.clobber) {
        for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
        c.compare(0, 17, 29);
        c.set_cr(6, 0x5);
        c.set_f(1, 3.5 + calls);
    }
    c.set(3, found->second);
    s.pc = c.lr();
    return true;
}

// The base world: zeroed records, every global, the tables.
void world(Cpu& c) {
    c.s.memory.copy_in(zero_from, std::vector<std::uint8_t>(zero_to - zero_from, 0));
    c.s.memory.copy_in(high, std::vector<std::uint8_t>(0x2000, 0));
    for (const auto slot : all_slots) {
        c.put32(c.r(2) + std::uint32_t(slot), cell(slot));
        c.put32(cell(slot), global_value(slot));
    }
    c.put8(cell(-0x7a68), 0);
    c.put8(cell(-0x7ab0), 0);
    c.put32(c.r(2) - 0x5618, pool);
    c.put32(cell(-0x7b9c), rows);
    c.put32(cell(-0x7be4), 5);
    c.put32(cell(-0x7be0), link_records);
    c.put32(cell(-0x7be8), clubs);
    c.put32(cell(-0x7520), nations);
    c.put32(cell(-0x7b7c), data + 0x1f00);
    c.put32(cell(-0x7b78), data + 0x1f10);
    c.put32(cell(-0x7a70), 3);
}

struct Rng {
    std::uint32_t state;
    std::uint32_t next() {
        state = state * 1664525u + 1013904223u;
        return state >> 8;
    }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    bool chance(unsigned in) { return below(in) == 0; }
    template <class T> T pick(std::initializer_list<T> list) { return *(list.begin() + below(unsigned(list.size()))); }
};

using Setup = std::function<void(Cpu&)>;
struct Collector {
    std::vector<Fixture> fixtures;
    void add(std::uint32_t entry, std::string text, Setup setup, Script script = {}, bool overflow = false,
             unsigned results = 1) {
        Fixture f;
        f.entry = entry;
        f.description = std::move(text);
        f.setup = [setup = std::move(setup)](Cpu& c) {
            world(c);
            setup(c);
        };
        f.stub = [script](State& s) { return scripted(s, script); };
        f.summary_overflow = overflow;
        f.results = results;
        fixtures.push_back(std::move(f));
    }
};

// Nation choices: a slot's value, or a miss.
std::uint32_t nation_of(std::int32_t slot, bool high_word = false) {
    return slot ? global_value(slot) : high_word ? high_miss : miss;
}

// --- Value tests -----------------------------------------------------------
void value_tests(Collector& out) {
    for (const std::uint32_t entry : {0x100c7a00u, 0x100c7a20u})
        for (std::uint32_t value = 0; value < 256; ++value)
            for (const std::uint32_t junk : {0u, 0x12345600u, 0x80000000u, 0xffffff00u})
                out.add(entry, "value " + std::to_string(value) + " junk " + hex_address(junk),
                        [=](Cpu& c) { c.set(3, junk | value); }, {}, (value + (junk >> 28)) % 3 == 0);
    for (const std::uint32_t byte : {0u, 1u, 2u, 0x80u, 0xffu})
        out.add(0x100b8b70, "flag byte " + std::to_string(byte), [=](Cpu& c) { c.put8(cell(-0x7ab0), byte); });
    // The two 2-bit fields: every old byte pattern against every value byte.
    for (const std::uint32_t old : {0u, 3u, 0xcu, 0xfu, 0x5au, 0xffu, 0x80u, 0x41u})
        for (std::uint32_t value = 0; value < 256; value += (value < 16 ? 1 : 7))
            for (const std::uint32_t junk : {0u, 0xabcd0000u})
                out.add(0x100ce200, "old " + std::to_string(old) + " value " + std::to_string(value),
                        [=](Cpu& c) {
                            c.put8(record_at(0) + 0x6a, old);
                            c.set(3, record_at(0));
                            c.set(4, junk | value);
                        }, {}, false, 0);
}

// --- Nation chains -----------------------------------------------------------
// A club's nation (field +0x68) against a chain's slots, and the linked-record
// variants with the club one record away at +0x2c.
void nation_chains(Collector& out) {
    const std::vector<std::pair<std::uint32_t, std::vector<std::int32_t>>> club_chains{
        {0x100caa10, {-0x7628, -0x762c, -0x7630, -0x7634, -0x7638}},
        {0x100caa90, {-0x763c, -0x7640, -0x7644, -0x7648, -0x764c}},
        {0x100de320, {-0x7a10, -0x7a00, -0x7af4, -0x7a14, -0x7a18, -0x7a28}}};
    for (const auto& [entry, slots] : club_chains)
        for (const bool overflow : {false, true})
            for (const bool null_club : {false, true}) {
                if (null_club && entry != 0x100de320) continue;  // only 0x100de320 tests for null
                auto nations_to_try = slots;
                nations_to_try.push_back(0);
                for (const auto nation_slot : nations_to_try)
                    for (const bool high_word : {false, true})
                        out.add(entry, "club nation " + std::to_string(nation_slot) + (high_word ? " high miss" : ""),
                                [=](Cpu& c) {
                                    c.put32(record_at(0) + 0x68, nation_of(nation_slot, high_word));
                                    c.set(3, null_club ? 0 : record_at(0));
                                }, {}, overflow);
            }
    const std::vector<std::pair<std::uint32_t, std::vector<std::int32_t>>> linked_chains{
        {0x100dbb20, {-0x7508}}, {0x100dbbe0, {-0x7508}},
        {0x100dbb60, {-0x7508, -0x7a00, -0x7590}},
        {0x100de260, {-0x7a10, -0x7a00, -0x7af4, -0x7a14, -0x7a18, -0x7a28}},
        {0x100ca7d0, {}}};
    for (const auto& [entry, slots] : linked_chains)
        for (const bool overflow : {false, true})
            for (const unsigned shape : {0u, 1u, 2u}) {  // null holder, null linked, linked set
                auto choices = slots;
                choices.push_back(0);
                for (const auto nation_slot : choices)
                    for (const std::uint32_t status : {0u, 2u, 3u, 0x82u})
                      for (const bool high_word : {false, true}) {
                        if (entry == 0x100ca7d0 && shape == 0) continue;  // no null test
                        if (entry != 0x100ca7d0 && status != 0) continue;
                        if (entry == 0x100ca7d0 && (nation_slot != 0 || high_word)) continue;
                        out.add(entry, "linked nation " + std::to_string(nation_slot) + " shape " + std::to_string(shape) +
                                           " status " + std::to_string(status) + (high_word ? " high miss" : ""),
                                [=](Cpu& c) {
                                    c.put32(record_at(1) + 0x68, nation_of(nation_slot, high_word));
                                    c.put8(record_at(1) + 0x11d, status);
                                    c.put32(record_at(0) + 0x2c, shape == 1 ? 0 : record_at(1));
                                    c.set(3, shape == 0 ? 0 : record_at(0));
                                }, {}, overflow);
                    }
            }
    // The long id chain: the linked nation must be -0x7a18's, then the holder's id one of 23.
    for (const bool overflow : {false, true})
        for (const unsigned shape : {0u, 1u, 2u, 3u})  // null, no linked, wrong nation, right nation
            for (const bool high_word : {false, true})
                for (int k = -1; k < 23; ++k) {
                    const std::int32_t id_slot = k < 0 ? 0 : -0x75cc - 4 * k;
                    out.add(0x100ca800, "id chain k=" + std::to_string(k) + " shape " + std::to_string(shape) + (high_word ? " high miss" : ""),
                            [=](Cpu& c) {
                                c.put32(record_at(1) + 0x68, shape == 2 ? (high_word ? high_miss : miss) : global_value(-0x7a18));
                                c.put32(record_at(0) + 0x2c, shape == 1 ? 0 : record_at(1));
                                c.put32(record_at(0), nation_of(id_slot, high_word));
                                c.set(3, shape == 0 ? 0 : record_at(0));
                            }, {}, overflow);
                }
}

// --- Lists -----------------------------------------------------------------------
void lists(Collector& out) {
    const std::uint32_t a = 0x60001000, b = 0x80000000, d = 0xffffffff;
    // Club list (+0x5c, three ids): the first equal id is zeroed.
    for (const auto& ids : std::array<std::array<std::uint32_t, 3>, 5>{{{a, b, d}, {b, a, d}, {d, b, a}, {a, a, a}, {0, 0, 0}}})
        for (const std::uint32_t id : {0u, a, b, d, 0x1234u})
            for (const unsigned shape : {0u, 1u, 2u})
                for (const bool overflow : {false, true})
                    out.add(0x100d9db0, "club list id " + hex_address(id) + " shape " + std::to_string(shape),
                            [=](Cpu& c) {
                                for (unsigned i = 0; i < 3; ++i) c.put32(record_at(0) + 0x5c + 4 * i, ids[i]);
                                c.set(3, shape == 0 ? 0 : record_at(0));
                                c.set(4, id);
                            }, {}, overflow, 0);
    // Preferences lists: removal from the three lists and the two keyed counts.
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 3> removals{
        {{0x100d9e10, 0x1c}, {0x100d9e90, 0x28}, {0x100da530, 0x4}}};
    for (const auto& [entry, first] : removals)
        for (const auto& ids : std::array<std::array<std::uint32_t, 3>, 5>{{{a, b, d}, {b, a, d}, {d, b, a}, {a, a, a}, {0, 0, 0}}})
            for (const std::uint32_t id : {0u, a, b, d, 0x1234u})
                for (const unsigned shape : {0u, 1u, 2u, 3u})  // null person, null preferences, normal
                    for (const bool overflow : {false, true})
                        out.add(entry, "preferences list id " + hex_address(id) + " shape " + std::to_string(shape),
                                [=](Cpu& c) {
                                    for (unsigned i = 0; i < 3; ++i) c.put32(record_at(1) + first + 4 * i, ids[i]);
                                    c.put32(record_at(0) + 0x18, shape == 2 ? 0 : record_at(1));
                                    c.set(3, shape == 0 ? 0 : record_at(0));
                                    c.set(4, id);
                                }, {}, overflow, 0);
    // Keyed counts: three staff records with keys, each slot null or set.
    for (const auto& [entry, first] : std::array<std::pair<std::uint32_t, std::uint32_t>, 2>{
             {{0x100d9f10, 0x28}, {0x100d9fb0, 0x1c}}})
        for (unsigned present = 0; present < 8; ++present)
            for (unsigned equal = 0; equal < 8; ++equal)
                for (const unsigned shape : {0u, 1u, 2u, 3u, 4u})  // null person, null preferences, null key, normal
                    for (const bool overflow : {false, true}) {
                        if (shape == 4 && (present != 7 || equal != 7)) continue;
                        out.add(entry, "keyed present " + std::to_string(present) + " equal " + std::to_string(equal) +
                                           " shape " + std::to_string(shape),
                                [=](Cpu& c) {
                                    for (unsigned i = 0; i < 3; ++i) {
                                        c.put32(record_at(2 + i) + 0x24, (equal >> i & 1) ? 0x55 : 0x56);
                                        c.put32(record_at(1) + first + 4 * i, (present >> i & 1) ? record_at(2 + i) : 0);
                                    }
                                    c.put32(record_at(0) + 0x18, shape == 1 ? 0 : record_at(1));
                                    c.set(3, shape == 0 ? 0 : record_at(0));
                                    c.set(4, shape == 2 ? 0 : 0x55);
                                }, {}, overflow);
                    }
    // Collections: which of the three slots are set, output buffer and counter.
    for (const auto& [entry, first] : std::array<std::pair<std::uint32_t, std::uint32_t>, 2>{
             {{0x100dafa0, 0x1c}, {0x100db060, 0x28}}})
        for (unsigned present = 0; present < 8; ++present)
            for (const unsigned shape : {0u, 1u, 2u, 3u, 4u})  // null person, null preferences, null out, normal, junk count
                for (const bool overflow : {false, true})
                    out.add(entry, "collect present " + std::to_string(present) + " shape " + std::to_string(shape),
                            [=](Cpu& c) {
                                for (unsigned i = 0; i < 3; ++i)
                                    c.put32(record_at(1) + first + 4 * i, (present >> i & 1) ? 0x60005000 + i : 0);
                                c.put32(record_at(0) + 0x18, shape == 1 ? 0 : record_at(1));
                                c.put8(record_at(5), shape == 4 ? 0x7f : 2);
                                c.set(3, shape == 0 ? 0 : record_at(0));
                                c.set(4, shape == 2 ? 0 : record_at(6));
                                c.set(5, record_at(5));
                            }, {}, overflow, 0);
    // Position masks over a squad of 50.
    Rng rng{0x13572468};
    for (unsigned n = 0; n < 300; ++n) {
        std::array<std::uint32_t, 50> members{};
        for (auto& member : members) {
            switch (rng.below(5)) {
            case 0: member = 0; break;
            case 1: member = record_at(1); break;                                    // no player
            case 2: member = record_at(2); break;                                    // goalkeeper
            case 3: member = record_at(3); break;                                    // sweeper
            default: member = record_at(4); break;                                   // nothing natural
            }
        }
        const std::uint32_t wanted = rng.pick<std::uint32_t>({0x1, 0x2, 0x10, 0x4, 0});
        out.add(0x100db120, "squad " + std::to_string(n),
                [=](Cpu& c) {
                    c.put32(record_at(2) + 0x14, record_at(10));
                    c.put32(record_at(3) + 0x14, record_at(11));
                    c.put32(record_at(4) + 0x14, record_at(12));
                    c.put8(record_at(10) + 0xf, 20);
                    c.put8(record_at(11) + 0x10, 20);
                    for (unsigned i = 0; i < 50; ++i) c.put32(record_at(0) + 0x98 + 4 * i, members[i]);
                    c.set(3, record_at(0));
                    c.set(4, wanted);
                }, {}, n % 5 == 0);
    }
    // 0x100d9440: with and without a record.
    for (const std::uint32_t record : {0u, record_at(0), high + 0x40})
        for (const bool clobber : {false, true})
            out.add(0x100d9440, "forward " + hex_address(record), [=](Cpu& c) { c.set(3, record); }, {.clobber = clobber});
}

// --- Club link rows -------------------------------------------------------------------
// Every row holds the same two links (so any club index finds them); the club index
// is below the count (5) or not, positive, -1 or INT_MIN (which wraps to row 0).
void links(Collector& out) {
    const std::array<std::uint32_t, 7> indexes{0, 2, 4, 5, 6, 0xffffffff, 0x80000000};
    constexpr std::uint32_t first_key = 0x40, second_key = 0x41;
    for (const std::uint32_t entry : {0x100ddf30u, 0x100ddfc0u, 0x100de090u, 0x100de160u})
        for (const auto club_index : indexes)
            for (const bool null_club : {false, true})
                for (const auto other_index : entry == 0x100ddfc0 ? std::vector<std::uint32_t>(indexes.begin(), indexes.end())
                                                                     : std::vector<std::uint32_t>{0})
                    for (const bool null_other : {false, true})
                        for (const std::uint32_t key : {first_key, second_key, 0x42u, 0xffffffffu})
                            for (const std::uint32_t first_index : {0xffffffffu, 1u})
                                for (const std::uint32_t second_index : {0xffffffffu, 2u}) {
                                    if (null_other && (entry != 0x100ddfc0 || other_index != 0)) continue;
                                    if (null_club && (club_index != 0 || key != first_key)) continue;
                                    out.add(entry,
                                            "links club " + hex_address(club_index) + " other " + hex_address(other_index) +
                                                " key " + hex_address(key) + " idx " + hex_address(first_index) + "," +
                                                hex_address(second_index) + (null_club ? " null club" : "") +
                                                (null_other ? " null other" : ""),
                                            [=](Cpu& c) {
                                                c.put32(record_at(0), club_index);
                                                c.put32(record_at(1), other_index);
                                                for (std::uint32_t r = 0xffffffff; r != 7; ++r) {
                                                    const auto row = rows + r * 0x64;
                                                    c.put32(row + 0x3c, first_index);
                                                    c.put32(row + 0x4c, first_key);
                                                    c.put32(row + 0x50, second_index);
                                                    c.put32(row + 0x60, second_key);
                                                }
                                                for (unsigned r = 0; r < 4; ++r) c.put32(link_records + r * 0x70, 0x9000 + r);
                                                c.set(3, null_club ? 0 : record_at(0));
                                                c.set(4, entry == 0x100ddfc0 ? (null_other ? 0 : record_at(1)) : key);
                                                c.set(5, 0x77);
                                            }, {}, club_index == 0 && key == second_key, entry == 0x100de160 ? 1 : 0);
                                }
}

// --- Nation rules and the person/club status rule ---------------------------------------
void rules(Collector& out) {
    // 0x100d33d0: the flag byte, the nation (the first word, an excluded one, the
    // nation record's word, or none), the nation record and its word.
    const std::vector<std::int32_t> nation_choices{-0x7b68, -0x75a8, -0x75b0, -0x7590, -0x758c, -0x7a70, 0};
    for (const std::uint32_t flag : {0u, 1u, 2u})
        for (const auto nation_slot : nation_choices)
            for (const std::uint32_t record : {0u, nations})
                for (const std::uint32_t record_word : {3u, 4u, 0x80000003u})
                    for (const bool overflow : {false, true})
                        out.add(0x100d33d0, "rule a flag " + std::to_string(flag) + " nation " + std::to_string(nation_slot) +
                                                " record " + hex_address(record) + " word " + std::to_string(record_word),
                                [=](Cpu& c) {
                                    c.put8(cell(-0x7a68), flag);
                                    c.put32(record_at(0) + 0x68, nation_of(nation_slot, overflow));
                                    c.put32(record_at(0) + 0x74, record);
                                    c.put32(nations + 8, record_word);
                                    c.set(3, record_at(0));
                                }, {}, overflow);
    // A null club reports the assertion (scripted, it returns) and then reads the null
    // club's fields: page 0 is mapped for these cases so the rules go on to answer.
    for (const std::uint32_t nation_word : {0u, 0x1234u, 0x80001234u})
        for (const std::uint32_t entry : {0x100d33d0u, 0x100d34c0u})
            out.add(entry, "null club " + std::to_string(nation_word),
                    [=](Cpu& c) {
                        c.s.memory.map(0, 0x1000);
                        c.put32(0x68, nation_word);
                        c.set(3, 0);
                    });
    for (const auto nation_slot : std::vector<std::int32_t>{-0x7a10, -0x7a1c, -0x7574, -0x757c, -0x7578, 0, -0x7a00})
        for (const bool clobber : {false, true})
            for (const bool overflow : {false, true})
                out.add(0x100d34c0, "rule b nation " + std::to_string(nation_slot),
                        [=](Cpu& c) {
                            c.put32(record_at(0) + 0x68, nation_of(nation_slot, overflow));
                            c.set(3, record_at(0));
                        }, {.clobber = clobber}, overflow);
    // 0x100d3040: every way to the club contract and its three tests.
    const std::uint32_t primary = record_at(8), club_contract = record_at(9), person = record_at(0);
    for (const std::uint32_t the_person : {0u, person, high + 0x700})
        for (const std::uint32_t the_club : {0u, record_at(6), high + 0x800})
            for (const std::uint32_t primary_answer : {0u, primary, high + 0x500})
                for (const std::uint32_t index : {0u, 3u, 0xffffffffu})
                    for (const std::uint32_t contract_answer : {0u, club_contract})
                        for (const std::uint32_t flags : {0u, 1u, 0xfeu, 0xffu})
                            for (const std::uint32_t club_answer : {0u, 1u, 0x100u, 0x80u})
                                for (const std::uint32_t role : {0xfu, 0x10u})
                                    for (const bool clobber : {false, true}) {
                                        if (!the_club && !primary_answer && (index || flags != 1)) continue;
                                        if (the_club && (primary_answer || index)) continue;
                                        if (!the_person && (the_club || role != 0xf || club_answer != 0)) continue;
                                        if (!contract_answer && (flags != 1 || club_answer != 0 || role != 0xf)) continue;
                                        out.add(0x100d3040,
                                                "status rule person " + hex_address(the_person) + " club " + hex_address(the_club) +
                                                    " primary " + hex_address(primary_answer) + " index " + hex_address(index) +
                                                    " contract " + hex_address(contract_answer) + " flags " + std::to_string(flags) +
                                                    " answer " + std::to_string(club_answer) + " role " + std::to_string(role),
                                                [=](Cpu& c) {
                                                    c.put32(primary + 4, index);
                                                    c.put8(club_contract + 0x5c, flags);
                                                    if (the_person) c.put8(the_person + 0x62, role);
                                                    c.set(3, the_person);
                                                    c.set(4, the_club);
                                                }, {.contract = primary_answer, .club_contract = contract_answer, .club_answer = club_answer, .clobber = clobber},
                                                clobber && flags == 1);
                                    }
}

// --- Comparators ------------------------------------------------------------------------
void comparators(Collector& out) {
    const std::array<std::int16_t, 7> values{-32768, -1000, -1, 0, 1, 700, 32767};
    const auto element = [](unsigned n) { return data + 0x1800 + n * 4; };
    // 0x100cd330: pointers to club pointers.
    for (const bool null_a : {false, true})
        for (const bool null_b : {false, true})
            for (const auto a : values)
                for (const auto b : values)
                    for (const bool overflow : {false, true}) {
                        if ((null_a || null_b) && (a != 0 || b != 1) ) continue;
                        out.add(0x100cd330, "club pointers " + std::to_string(a) + " " + std::to_string(b),
                                [=](Cpu& c) {
                                    c.put32(element(0), null_a ? 0 : record_at(0));
                                    c.put32(element(1), null_b ? 0 : record_at(1));
                                    c.put16(record_at(0) + 0x1ec, std::uint16_t(a));
                                    c.put16(record_at(1) + 0x1ec, std::uint16_t(b));
                                    c.set(3, element(0));
                                    c.set(4, element(1));
                                }, {}, overflow);
                    }
    // 0x100cd390: club indexes (negative and zero skip the table).
    const std::array<std::uint32_t, 6> indexes{0xfffffffdu, 0xffffffffu, 0, 1, 2, 0x80000000u};
    for (const auto ia : indexes)
        for (const auto ib : indexes)
            for (const auto a : values)
                for (const auto b : values)
                    out.add(0x100cd390, "club indexes " + hex_address(ia) + " " + hex_address(ib) + " " + std::to_string(a) + " " + std::to_string(b),
                            [=](Cpu& c) {
                                c.put32(element(0), ia);
                                c.put32(element(1), ib);
                                // A row index of INT_MIN or -3 is skipped (not above -1).
                                for (unsigned row = 0; row < 3; ++row)
                                    c.put16(clubs + row * 0x248 + 0x1ec, std::uint16_t(row == ia ? a : (row == ib ? b : 0)));
                                c.set(3, element(0));
                                c.set(4, element(1));
                            }, {}, (a ^ b) & 1);
    // 0x100cd740: holders with players; the players' 16-bit values at +0xa.
    for (const unsigned shape_a : {0u, 1u, 2u})  // null holder, no player, player
        for (const unsigned shape_b : {0u, 1u, 2u})
            for (const auto a : values)
                for (const auto b : values) {
                    if ((shape_a != 2 || shape_b != 2) && (a != -1 || b != 700)) continue;
                    out.add(0x100cd740, "holders " + std::to_string(shape_a) + std::to_string(shape_b) + " " + std::to_string(a) + " " + std::to_string(b),
                            [=](Cpu& c) {
                                c.put32(element(0), shape_a == 0 ? 0 : record_at(0));
                                c.put32(element(1), shape_b == 0 ? 0 : record_at(1));
                                c.put32(record_at(0) + 0x14, shape_a == 2 ? record_at(2) : 0);
                                c.put32(record_at(1) + 0x14, shape_b == 2 ? record_at(3) : 0);
                                c.put16(record_at(2) + 0xa, std::uint16_t(a));
                                c.put16(record_at(3) + 0xa, std::uint16_t(b));
                                c.set(3, element(0));
                                c.set(4, element(1));
                            }, {}, shape_a == shape_b);
                }
    // 0x100ddea0: players' +4 values, then +0xa when equal.
    for (const unsigned shape_a : {0u, 1u, 2u})
        for (const unsigned shape_b : {0u, 1u, 2u})
            for (const auto a4 : {std::int16_t(-3), std::int16_t(0), std::int16_t(2)})
                for (const auto b4 : {std::int16_t(-3), std::int16_t(0), std::int16_t(2)})
                    for (const auto ax : {std::int16_t(-1), std::int16_t(4)})
                        for (const auto bx : {std::int16_t(-1), std::int16_t(7)}) {
                            if ((shape_a != 2 || shape_b != 2) && (a4 != 0 || b4 != 0 || ax != 4 || bx != 7)) continue;
                            out.add(0x100ddea0, "players " + std::to_string(shape_a) + std::to_string(shape_b) + " " + std::to_string(a4) + "," + std::to_string(b4) + "," + std::to_string(ax) + "," + std::to_string(bx),
                                    [=](Cpu& c) {
                                        c.put32(element(0), shape_a == 0 ? 0 : record_at(0));
                                        c.put32(element(1), shape_b == 0 ? 0 : record_at(1));
                                        c.put32(record_at(0) + 0x14, shape_a == 2 ? record_at(2) : 0);
                                        c.put32(record_at(1) + 0x14, shape_b == 2 ? record_at(3) : 0);
                                        c.put16(record_at(2) + 4, std::uint16_t(a4));
                                        c.put16(record_at(3) + 4, std::uint16_t(b4));
                                        c.put16(record_at(2) + 0xa, std::uint16_t(ax));
                                        c.put16(record_at(3) + 0xa, std::uint16_t(bx));
                                        c.set(3, element(0));
                                        c.set(4, element(1));
                                    }, {}, shape_a == 2 && a4 == 0);
                        }
    // 0x100cde70: keys behind +4, then the 16-bit values at +0x18.
    for (const bool null_a : {false, true})
        for (const bool null_b : {false, true})
            for (const std::uint32_t word_a : {5u, 6u, 0u})
                for (const std::uint32_t word_b : {5u, 6u, 0x80000000u})
                    for (const auto a18 : {std::int16_t(-1), std::int16_t(0), std::int16_t(5)})
                        for (const auto b18 : {std::int16_t(-1), std::int16_t(0), std::int16_t(7)}) {
                            if ((null_a || null_b) && (word_a != 5 || word_b != 5 || a18 != 0 || b18 != 0)) continue;
                            out.add(0x100cde70, "keys " + std::to_string(null_a) + std::to_string(null_b) + " " + std::to_string(word_a) + "," + std::to_string(word_b) + "," + std::to_string(a18) + "," + std::to_string(b18),
                                    [=](Cpu& c) {
                                        c.put32(record_at(0) + 4, null_a ? 0 : record_at(2));
                                        c.put32(record_at(1) + 4, null_b ? 0 : record_at(3));
                                        c.put32(record_at(2), word_a);
                                        c.put32(record_at(3), word_b);
                                        c.put16(record_at(0) + 0x18, std::uint16_t(a18));
                                        c.put16(record_at(1) + 0x18, std::uint16_t(b18));
                                        c.set(3, record_at(0));
                                        c.set(4, record_at(1));
                                    }, {}, null_a || word_a == 0);
                        }
}

// --- The person/club gate, its classes and values ---------------------------------------
// One seeded world per case: clubs with nations drawn from the gate's slots, statuses,
// links, the person's and the other's clubs, a contract and a date. Each choice picks
// an interesting value often enough for the deep paths to run.
struct GateWorld {
    std::uint32_t seed;
    std::uint32_t entry;
    void operator()(Cpu& c) const {
        Rng r{seed};
        static const std::vector<std::int32_t> palette{-0x74f4, -0x7af4, -0x7a14, -0x750c, -0x75c4, -0x7504, -0x75c8,
                                                        -0x7a10, -0x7a1c, -0x757c, -0x7578, -0x7574, -0x7650, -0x7654,
                                                        -0x7658, -0x79fc, 0, 0};
        const auto nation = [&] { return nation_of(palette[r.below(unsigned(palette.size()))], r.chance(2)); };
        constexpr unsigned club_count = 8;
        // Half the clubs sit above 0x80000000, so pointer compares (cmplw) see both signs.
        const auto club = [](unsigned n) { return n < 4 ? record_at(2 + n) : high + 0x400 * (n - 4); };
        for (unsigned n = 0; n < club_count; ++n) {
            c.put32(club(n) + 0x68, nation());
            c.put8(club(n) + 0x11d, r.pick<std::uint32_t>({0, 1, 2, 2, 3}));
            c.put32(club(n) + 0x74, r.chance(2) ? nations + 3 * 0xc8 : r.pick<std::uint32_t>({0, pool, high + 0x1800}));
            c.put32(club(n) + 0x2c, r.chance(4) ? 0 : club(r.below(club_count)));
            c.put32(club(n) + 0x0, r.pick<std::uint32_t>({0, 3, 0xfffffff0u, 0x80000000u, 0xffffffffu}));
            c.put16(club(n) + 0x1ec, r.pick<std::uint32_t>({0, 100, 0x2000, 0x3600, 0x7fff, 0x8000}));
        }
        const std::array<std::uint32_t, 6> halves{0, 1, 0x7ce, 0xffff, 0x8000, 0x7fff};
        const auto pick_half = [&] { return halves[r.below(6)]; };
        const auto person = [&](std::uint32_t at, unsigned first, unsigned second, unsigned current) {
            c.put32(at + 0xc, club(first));
            c.put32(at + 0x10, r.chance(3) ? 0 : club(second));
            c.put32(at + 0x24, r.chance(4) ? 0 : club(current));
            c.put32(at + 0x14, at + 0x200);
            c.put32(at + 0x1c, at + 0x240);
            for (const std::uint32_t offset : {0x8u, 0xau, 0xcu}) c.put16(at + 0x200 + offset, r.pick<std::uint32_t>({0x8000, 0xfff0, 0, 7, 0x3000, 0x7fff}));
            for (const std::uint32_t offset : {0x28u, 0x2au, 0x2cu}) c.put16(at + 0x240 + offset, r.pick<std::uint32_t>({0x8000, 0xfff0, 0, 7, 0x3000, 0x7fff}));
        };
        const unsigned shared = r.below(club_count);
        person(record_at(0), r.below(club_count), r.below(club_count), shared);
        person(record_at(1), r.chance(4) ? shared : r.below(club_count), r.below(club_count),
               r.chance(3) ? shared : r.below(club_count));
        // The other's current club may be the person's (class 0).
        if (r.chance(5)) c.put32(record_at(1) + 0x24, c.u32(record_at(0) + 0x24));
        if (r.chance(5)) c.put32(club(1) + 0x2c, club(1));
        c.put32(cell(-0x7be4), r.pick<std::uint32_t>({5, 0x40, 0x7fffffff, 0x80000010u}));
        // The contract's date fields and the date the scripted call answers.
        const auto contract = record_at(20);
        c.put16(contract + 0x28, pick_half());
        c.put16(contract + 0x2a, pick_half());
        c.put16(date_words, pick_half());
        c.put16(date_words + 2, r.chance(2) ? c.u16(contract + 0x2a) : pick_half());
        c.put32(date_words + 4, 0x1234abcd);
        if (r.chance(3)) c.put16(date_words, c.u16(contract + 0x28));
        const std::uint32_t flag = r.pick<std::uint32_t>({0, 1, 1, 1, 0x100, 0x80, 0xff});
        const bool flag_set = std::int8_t(flag) != 0;
        // A null first club (0x100cacbc, 0x100cad2c) or player (0x100ce1ac) where the routine
        // would not fault on it: the gate dereferences the first club only when the club's
        // nation is in its first chain or is -0x79fc's, so those nations are replaced.
        const auto avoid_nations = [&](std::uint32_t club_address) {
            if (club_address) c.put32(club_address + 0x68, miss);
        };
        if (entry != 0x100ca6a0 && r.chance(5)) {
            c.put32(record_at(0) + 0xc, 0);
        }
        if (entry == 0x100ce0a0 || entry == 0x100cdef0) {
            if (r.chance(5)) {
                c.put32(record_at(1) + 0xc, 0);
                avoid_nations(c.u32(record_at(0) + 0xc));
                const auto current = c.u32(record_at(0) + 0x24);
                if (current) avoid_nations(c.u32(current + 0x2c));
            }
            if ((entry == 0x100ce0a0 || flag_set) && r.chance(4)) c.put32(record_at(0) + 0x14, 0);
        }
        std::uint32_t argument_club = club(r.below(club_count));
        switch (r.below(8)) {
        case 0: argument_club = 0; break;
        case 1: argument_club = c.u32(record_at(0) + 0xc); break;
        case 2: argument_club = c.u32(record_at(0) + 0x10); break;
        default: break;
        }
        if (entry == 0x100cab10 && c.u32(record_at(0) + 0xc) == 0) avoid_nations(argument_club);
        c.set(3, record_at(0));
        switch (entry) {
        case 0x100ca6a0:
        case 0x100cab10: c.set(4, argument_club); break;
        default: c.set(4, r.chance(10) ? 0 : record_at(1)); break;
        }
        c.set(5, flag);
    }
};

void gates(Collector& out) {
    const auto script = [](Rng& r) {
        return Script{.contract = r.chance(5) ? 0 : record_at(20), .clobber = r.chance(2)};
    };
    const std::array<std::pair<std::uint32_t, unsigned>, 4> routines{
        {{0x100ca6a0, 400}, {0x100cab10, 1000}, {0x100ce0a0, 600}, {0x100cdef0, 400}}};
    for (const auto& [entry, count] : routines) {
        Rng seeds{entry};
        for (unsigned n = 0; n < count; ++n) {
            const auto seed = seeds.next() | 1;
            Rng r{seed ^ 0x5a5a5a};
            out.add(entry, "gate seed " + hex_address(seed), GateWorld{seed, entry}, script(r), n % 5 == 0);
        }
    }
}
}

std::vector<Fixture> lookup_fixtures() {
    Collector out;
    value_tests(out);
    nation_chains(out);
    lists(out);
    links(out);
    rules(out);
    comparators(out);
    gates(out);
    return std::move(out.fixtures);
}
}
