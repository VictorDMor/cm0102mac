// cm-database-checks fixtures for DATABASE.CPP blocks 1-2: the attribute
// ratings and preference lists (every value a rating's inputs can take;
// every hit position, miss, duplicate, null record and extreme id), the
// contract-flag queries (CONTRACT_MANAGER.CPP's two lookups scripted, every
// path to them, records on both sides of 0x80000000, clobbered volatiles)
// and the position mask (every rating that changes a test, plus mixes).
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <memory>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;
constexpr std::uint32_t record = data + 0x100, club = data + 0x200, person = data + 0x300,
                        preferences = data + 0x400, primary = data + 0x500, club_contract = data + 0x600,
                        player = data + 0x900, manager_word = data + 0xa00, last_primary_word = data + 0xa04,
                        table_word = data + 0xa08;
constexpr std::array ratings{0x100d2d70u, 0x100d2e60u, 0x100d2f50u};
constexpr std::array club_lists{0x100d9480u, 0x100d94d0u};
constexpr std::array person_lists{0x100d9520u, 0x100d9580u};
constexpr std::array person_club_lists{0x100da050u, 0x100da0b0u};
constexpr std::array contract_flags{0x100d3150u, 0x100d3230u, 0x100d3300u};
constexpr std::uint32_t position_mask = 0x100db1b0, primary_lookup = 0x10082f20, club_lookup = 0x10082f90;

struct Case {
    std::uint32_t entry = 0, r3 = 0, r4 = 0;
    std::int16_t field = 0;
    std::array<std::uint32_t, 3> slots{};
    bool null_preferences = false, summary_overflow = false, sticky_fpscr = false;
    // Contract queries: what the two lookups answer, and the club table.
    std::uint32_t primary_answer = 0, index = 0, table = 0, club_answer = 0;
    std::uint8_t flags = 0;
    bool clobber = false;
    // Position masks: the ratings from the goalkeeper's (+0xf) to +0x19.
    std::array<std::uint8_t, 11> ratings{};
    bool null_player = false;
};

void initialize(Cpu& c, const Case& t) {
    if (t.sticky_fpscr) for (const std::size_t bit : {1280u, 1286u, 1294u, 1295u}) c.s.wr<1>(bit, 1);
    // Ratings: the 16-bit value at both offsets the routines read.
    c.put16(record + 0x4, std::uint16_t(t.field));
    c.put16(record + 0x24, std::uint16_t(t.field));
    // Lists: the same three ids in every list of the club and the
    // preferences, so each routine sees them at its own offsets.
    for (unsigned i = 0; i < 3; ++i) {
        for (const std::uint32_t list : {0x5cu, 0x68u}) c.put32(club + list + 4 * i, t.slots[i]);
        for (const std::uint32_t list : {0x4u, 0x10u, 0x1cu, 0x28u}) c.put32(preferences + list + 4 * i, t.slots[i]);
    }
    c.put32(person + 0x18, t.null_preferences ? 0 : preferences);
    // Contract queries: the TOC words point at the manager's word, the
    // "last primary contract" word and the club table's base word.
    c.put32(c.r(2) - 0x7b7c, manager_word);
    c.put32(manager_word, data + 0xb00);
    c.put32(c.r(2) - 0x7b78, last_primary_word);
    c.put32(c.r(2) - 0x7be8, table_word);
    c.put32(table_word, t.table);
    if (t.primary_answer) c.put32(t.primary_answer + 0x4, t.index);
    if (t.club_answer) c.put8(t.club_answer + 0x5c, t.flags);
    // Position masks.
    c.put32(person + 0x14, t.null_player ? 0 : player);
    for (unsigned i = 0; i < t.ratings.size(); ++i) c.put8(player + 0xf + i, t.ratings[i]);
    c.set(3, t.r3);
    c.set(4, t.r4);
}

// CONTRACT_MANAGER.CPP's lookups, scripted: the call is recorded, answered
// from the case and, with `clobber`, leaves volatile registers and CR the
// caller must carry through.
bool lookup(State& s, const Case& t) {
    if (s.pc != primary_lookup && s.pc != club_lookup) return false;
    Cpu c(s);
    const auto calls = c.u32(trace_at);
    record_call(c);
    if (t.clobber) {
        for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + calls * 0x100 + r);
        c.compare(0, 17, 29);
        c.set_cr(6, 0x5);
        c.set_f(1, 3.5 + calls);
    }
    c.set(3, s.pc == primary_lookup ? t.primary_answer : t.club_answer);
    s.pc = c.lr();
    return true;
}

std::string describe(const Case& t) {
    std::string text = "r3=" + hex_address(t.r3) + " r4=" + hex_address(t.r4) + " field=" + std::to_string(t.field) +
                       " slots=" + hex_address(t.slots[0]) + ',' + hex_address(t.slots[1]) + ',' + hex_address(t.slots[2]) +
                       " null_preferences=" + std::to_string(t.null_preferences) +
                       " primary=" + hex_address(t.primary_answer) + " index=" + hex_address(t.index) +
                       " table=" + hex_address(t.table) + " club_contract=" + hex_address(t.club_answer) +
                       " flags=" + std::to_string(t.flags) + " clobber=" + std::to_string(t.clobber) +
                       " null_player=" + std::to_string(t.null_player) + " ratings=";
    for (const auto rating : t.ratings) text += std::to_string(rating) + ',';
    return text;
}

std::vector<Case> cases() {
    std::vector<Case> found;
    // Ratings: the byte (with junk above it, which the original drops) and
    // the 16-bit value across its range and the formula's clamps.
    const std::array<std::int16_t, 17> fields{-32768, -1000, -200, -100, -1, 0, 1, 50, 99, 100,
                                              150, 199, 200, 201, 255, 1000, 32767};
    for (const auto entry : ratings)
        for (const auto field : fields)
            for (int value = -128; value < 128; ++value) {
                Case t;
                t.entry = entry;
                t.r3 = record;
                t.r4 = (value & 0x0f ? 0x12345600u : 0u) | std::uint8_t(value);
                t.field = field;
                t.sticky_fpscr = (value & 0x1f) == 3;
                found.push_back(t);
            }
    // Lists: ids at each slot, absent or repeated, logical extremes.
    const std::uint32_t a = 0x60001000, b = 0x80000000, d = 0xffffffff;
    const std::array<std::array<std::uint32_t, 3>, 6> patterns{{
        {a, b, d}, {b, a, d}, {d, b, a}, {a, a, a}, {0, 0, 0}, {0x7fffffff, 1, 0x60000fff}}};
    const std::array<std::uint32_t, 5> ids{0, a, b, d, 0x60002000};
    for (const auto& slots : patterns)
        for (const auto id : ids)
            for (const bool null_record : {false, true})
                for (const bool null_preferences : {false, true})
                    for (const bool overflow : {false, true}) {
                        Case t;
                        t.slots = slots;
                        t.null_preferences = null_preferences;
                        t.summary_overflow = overflow;
                        for (const auto entry : club_lists) {
                            t.entry = entry;
                            t.r3 = id;
                            t.r4 = null_record ? 0 : club;
                            found.push_back(t);
                        }
                        for (const auto entry : person_lists) {
                            t.entry = entry;
                            t.r3 = id;
                            t.r4 = null_record ? 0 : person;
                            found.push_back(t);
                        }
                        for (const auto entry : person_club_lists) {
                            t.entry = entry;
                            t.r3 = null_record ? 0 : person;
                            t.r4 = id;
                            found.push_back(t);
                        }
                    }
    // Contract queries: every way to a lookup, the flag bytes that change a
    // test, and records on both sides of 0x80000000 (signed compares) with
    // and without XER[SO].
    const std::array<std::uint8_t, 9> flag_values{0, 1, 2, 4, 8, 9, 0xa, 0xf6, 0xff};
    for (const auto entry : contract_flags) {
        Case t;
        t.entry = entry;
        found.push_back(t);  // null person
        for (const bool overflow : {false, true})
            for (const std::uint32_t someone : {data + 0x700, high + 0x700})
                for (const bool clobber : {false, true}) {
                    t.summary_overflow = overflow;
                    t.r3 = someone;
                    t.clobber = clobber;
                    const auto with_contract = [&](Case u) {
                        u.club_answer = 0;
                        found.push_back(u);
                        for (const std::uint32_t answer : {club_contract, high + 0x600}) {
                            u.club_answer = answer;
                            for (const auto flags : flag_values) {
                                u.flags = flags;
                                found.push_back(u);
                            }
                        }
                    };
                    for (const std::uint32_t the_club : {0u, data + 0x800, high + 0x800}) {
                        t.r4 = the_club;
                        if (the_club) {
                            with_contract(t);
                            continue;
                        }
                        t.primary_answer = 0;
                        found.push_back(t);
                        for (const std::uint32_t answer : {primary, high + 0x500}) {
                            t.primary_answer = answer;
                            // The club address comes out 0, positive, negative or wrapped.
                            for (const auto& [index, table] : std::array<std::pair<std::uint32_t, std::uint32_t>, 4>{{
                                     {0, 0}, {3, data + 0x1000}, {3, 0x7ffffff0}, {0xffffffff, 0x100}}}) {
                                t.index = index;
                                t.table = table;
                                if (index == 0 && table == 0) found.push_back(t);
                                else with_contract(t);
                            }
                        }
                    }
                }
    }
    // Position masks: each rating alone at the values that change a test,
    // then pseudo-random mixes of them.
    const std::array<std::uint8_t, 9> values{0, 10, 11, 14, 15, 19, 20, 0x94, 0xff};
    {
        Case t;
        t.entry = position_mask;
        found.push_back(t);  // null person
        t.r3 = person;
        t.null_player = true;
        found.push_back(t);
        t.null_player = false;
        for (unsigned i = 0; i < t.ratings.size(); ++i)
            for (const auto value : values) {
                t.ratings = {};
                t.ratings[i] = value;
                found.push_back(t);
            }
        std::uint32_t seed = 0x2545f491;
        for (unsigned n = 0; n < 4000; ++n) {
            for (auto& rating : t.ratings) {
                seed = seed * 1664525u + 1013904223u;
                // Mostly low ratings, so the "no natural position" pass runs.
                rating = (seed >> 24) < 96 ? values[(seed >> 8) % values.size()] : std::uint8_t((seed >> 12) % 15);
            }
            t.summary_overflow = n % 7 == 0;
            found.push_back(t);
        }
    }
    return found;
}

}

std::vector<Fixture> first_fixtures() {
    std::vector<Fixture> found;
    for (const auto& t : cases()) {
        Fixture f;
        f.entry = t.entry;
        f.description = describe(t);
        f.setup = [t](Cpu& c) { initialize(c, t); };
        f.stub = [t](State& s) { return lookup(s, t); };
        f.summary_overflow = t.summary_overflow;
        found.push_back(std::move(f));
    }
    return found;
}
}
