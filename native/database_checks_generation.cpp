// cm-database-checks fixtures for DATABASE.CPP's generation block (database_checks.hpp).
// These routines draw from the game's random generator (rng_core, entry 0x1077b770, state word
// at TOC -0x34c4). Every case runs either with the draws scripted (the stub answers each call
// from the case's list, recording its registers, so every outcome of every draw is covered and the
// number, order and registers of the calls are compared) or with the real generator seeded
// from the case (the C++ owner runs on both sides, the draws and the state word compared).
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <memory>
#include <string>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;
constexpr std::uint32_t random_entry = 0x1077b770, random_state_slot = 0x34c4;
// The generator and the contract lookup read few registers: the trace of each stubbed call keeps its target, LR,
// the argument registers r3.., the non-volatile registers (a callee's prologue saves them) and CR, in record_call's
// format, so the compared memory holds exactly what those callees can see.
constexpr std::uint32_t draw_counter = data + 0x1f000;
void record_arguments(Cpu& c, unsigned arguments) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) {
        c.put32(at, value);
        at += 4;
    };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r < 3 + arguments; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}

// The draws a case scripts (empty: the real generator, seeded). The last answer repeats.
struct Draws {
    std::vector<std::uint32_t> values;
    std::uint32_t seed = 0;
};
bool scripted(State& s, const Draws& d) {
    if (s.pc != random_entry || d.values.empty()) return false;
    Cpu c(s);
    const auto index = c.u32(draw_counter);
    c.put32(draw_counter, index + 1);
    record_arguments(c, 0);
    c.set(3, d.values[std::min<std::size_t>(index, d.values.size() - 1)]);
    s.pc = c.lr();
    return true;
}
void seed_generator(Cpu& c, const Draws& d) { c.put32(c.r(2) - random_state_slot, d.seed); }

std::string text(const Draws& d) {
    std::string out = d.values.empty() ? "real seed " + std::to_string(d.seed) : "draws";
    for (const auto v : d.values) out += ' ' + std::to_string(v);
    return out;
}

// A small deterministic generator for the fixtures' choices.
struct Mix {
    std::uint32_t state;
    std::uint32_t next() { state = state * 1664525u + 1013904223u; return state >> 8; }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
    template <class T> T pick(std::initializer_list<T> list) { return *(list.begin() + below(std::uint32_t(list.size()))); }
};

Fixture make(std::uint32_t entry, std::string description, const Draws& d, std::function<void(Cpu&)> setup,
             bool overflow = false, std::function<bool(State&)> other = {}) {
    Fixture f;
    f.entry = entry;
    f.description = std::move(description) + ' ' + text(d);
    f.setup = [d, setup = std::move(setup)](Cpu& c) {
        c.put32(draw_counter, 0);
        seed_generator(c, d);
        setup(c);
    };
    f.stub = [d, other = std::move(other)](State& s) { return scripted(s, d) || (other && other(s)); };
    f.summary_overflow = overflow;
    return f;
}

// The draws of a case: a few scripted values from the interesting ones, or a real seed.
Draws script(Mix& mix, unsigned count) {
    Draws d;
    constexpr std::array<std::uint32_t, 8> edges{0, 1, 2, 16383, 16384, 32766, 32767, 10000};
    if (mix.below(4) == 0) {
        d.seed = mix.next() * 2654435761u;
        return d;
    }
    for (unsigned i = 0; i < count; ++i) d.values.push_back(mix.below(3) == 0 ? edges[mix.below(8)] : mix.below(32768));
    return d;
}

// Name pickers: a nation row table (0x24-byte rows), the pointer words the TOC slots point at
// and a table of 0x2c-byte entries.
constexpr std::uint32_t person_at = data + 0x200, rows_cell = data + 0x100, bound_a_cell = data + 0x110,
                        bound_b_cell = data + 0x114, names_a_cell = data + 0x120, names_b_cell = data + 0x124,
                        rows = data + 0x1000, names = data + 0x8000;
void name_world(Cpu& c, Mix& mix, std::uint32_t nation, std::int32_t count, std::int32_t first, std::int32_t bound) {
    c.put32(c.r(2) - 0x7580, rows_cell);
    c.put32(rows_cell, rows);
    c.put32(c.r(2) - 0x754c, bound_a_cell);
    c.put32(c.r(2) - 0x7550, bound_b_cell);
    c.put32(bound_a_cell, std::uint32_t(bound));
    c.put32(bound_b_cell, std::uint32_t(bound));
    c.put32(c.r(2) - 0x752c, names_a_cell);
    c.put32(c.r(2) - 0x7530, names_b_cell);
    c.put32(names_a_cell, names);
    c.put32(names_b_cell, names);
    c.put32(person_at + 0x68, nation);
    for (const std::uint32_t at : {0x18u, 0x1cu}) c.put32(rows + nation * 0x24 + at, std::uint32_t(count));
    for (const std::uint32_t at : {0x0u, 0x4u}) c.put32(rows + nation * 0x24 + at, std::uint32_t(first));
    for (std::uint32_t i = 0; i < 400; ++i)
        c.put8(names + 0x2c * i + 0x28, mix.below(3) == 0 ? mix.pick<std::uint32_t>({0, 1, 0x7f, 0x80, 0xff, 0xff}) : mix.below(256));
    // Entries below the table (negative indexes) get weights too.
    for (std::uint32_t i = 1; i <= 100; ++i) c.put8(names - 0x2c * i + 0x28, mix.below(256));
}

void add_name_pickers(std::vector<Fixture>& out) {
    Mix mix{0x5eed1234};
    for (const std::uint32_t entry : {0x100c42b0u, 0x100c4ba0u}) {
        const std::int32_t threshold = entry == 0x100c42b0 ? 15 : 10;
        const std::array<std::int32_t, 12> counts{-100, -1, 0, 1, threshold - 1, threshold, threshold + 1, threshold + 2, 30, 100, 120, -64};
        const std::array<std::int32_t, 9> bounds{-140, -5, 0, 1, 7, 100, 1400, 0x7fff, 0x12345};
        for (unsigned n = 0; n < 160; ++n) {
            const bool null_person = n % 20 == 3;
            const auto count = counts[n % counts.size()];
            auto draws = script(mix, 3);
            const auto nation = mix.below(4);
            const auto first = std::int32_t(mix.below(40)) - (n % 7 == 0 ? 20 : 0);
            const auto bound = bounds[mix.below(bounds.size())];
            // A bound over the table's size only works with small samples (index = bound * sample / 32768).
            if (bound > 1400 && count <= threshold) draws = Draws{{0, 1, 2, 20, 60}, 0};
            // Each case is built from its own sub-stream, so the world is the same on every run.
            const auto world = mix.next();
            out.push_back(make(entry, "name picker count " + std::to_string(count) + " bound " + std::to_string(bound) +
                                       (null_person ? " null person" : ""), draws, [=](Cpu& c) {
                Mix local{world};
                name_world(c, local, nation, count, first, bound);
                c.set(3, null_person ? 0 : person_at);
            }, n % 5 == 0));
        }
    }
}

// Three-slot lists: the pool (a cursor into 16-bit samples that wraps, a salt, an end), a club
// record with the lists at +0x5c/+0x68, a person's preferences record at +0x18 with its lists at
// +0x4, +0x10, +0x1c, +0x28, and the ids the lists hold.
constexpr std::uint32_t pool_start = data + 0x2000, pool_end_cell = data + 0x1f00, cursor_cell = data + 0x1f10,
                        salt_cell = data + 0x1f20, owner_at = data + 0x400,
                        preferences_at = data + 0x600, club_at = data + 0x700;
constexpr std::array<std::uint32_t, 4> ids{0x60001000, 0x80000010, 0x60001234, 0xfffffff0};

struct ListWorld {
    std::uint32_t cursor_step = 0;  // 0: at the start ... 3: past the end
    std::uint32_t salt = 0;
    bool null_preferences = false;
};
void pool_world(Cpu& c, Mix& mix, const ListWorld& w) {
    c.put32(c.r(2) - 0x7b84, cursor_cell);
    c.put32(c.r(2) - 0x7b90, salt_cell);
    c.put32(c.r(2) - 0x7b88, pool_end_cell);
    c.put32(c.r(2) - 0x7b8c, pool_start);
    c.put32(pool_end_cell, pool_start + 0x80);
    const std::array<std::uint32_t, 4> cursors{pool_start, pool_start + 0x7c, pool_start + 0x7e, pool_start + 0x80};
    c.put32(cursor_cell, cursors[w.cursor_step]);
    c.put32(salt_cell, w.salt);
    for (std::uint32_t i = 0; i < 0x50; ++i) c.put16(pool_start + 2 * i, mix.below(0x10000));
}
std::uint32_t choose_id(Mix& mix) { return mix.below(5) == 0 ? 0 : ids[mix.below(ids.size())]; }

void add_list_routines(std::vector<Fixture>& out) {
    struct Shape {
        std::uint32_t entry;
        std::array<std::uint32_t, 2> lists;  // the record's two lists (kept, opposite)
        bool club_record, self_check, holder_lists;
    };
    const std::array<Shape, 6> shapes{{
        {0x100d95e0, {0x5c, 0x68}, true, false, false},
        {0x100d97a0, {0x68, 0x5c}, true, false, false},
        {0x100d9960, {0x1c, 0x28}, false, false, false},
        {0x100d9b80, {0x28, 0x1c}, false, true, false},
        {0x100da110, {0x4, 0x10}, false, false, true},
        {0x100da320, {0x10, 0x4}, false, false, true},
    }};
    Mix mix{0x11a5eed};
    constexpr std::array<std::uint32_t, 8> salts{0, 1, 2, 0x7fff, 0xffff, 0x80000000u, 0xfffffff0u, 0xfffffffeu};
    constexpr std::array<std::uint32_t, 7> flags{0, 1, 0x7f, 0x80, 0xff, 0x100, 0x1234500};
    for (const auto& shape : shapes) {
        for (unsigned n = 0; n < 600; ++n) {
            ListWorld w{mix.below(4), mix.below(3) == 0 ? salts[mix.below(salts.size())] : mix.below(0x8000),
                        mix.below(12) == 0};
            const auto draws = script(mix, 1);
            const auto flag = flags[mix.below(flags.size())];
            // The id a list holds: a person's, or (for the person-club routines) the club's address.
            const auto id = shape.holder_lists ? (mix.below(10) == 0 ? 0 : club_at) : choose_id(mix);
            const bool null_owner = mix.below(15) == 0, self = mix.below(12) == 0;
            // Each of the three slots of both lists: empty or one of the ids; some runs fill the kept list.
            std::array<std::uint32_t, 6> slots{};
            const bool full = mix.below(2) == 0;
            for (unsigned i = 0; i < 6; ++i) slots[i] = (i < 3 && full) ? ids[mix.below(ids.size())] : choose_id(mix);
            if (mix.below(3) == 0) slots[mix.below(6)] = id;  // the id is already somewhere
            std::array<std::uint32_t, 3> holder{};
            for (auto& h : holder) h = mix.below(3) == 0 ? owner_at : choose_id(mix);
            const auto world = mix.next();
            out.push_back(make(shape.entry, "list case " + std::to_string(n), draws, [=](Cpu& c) {
                Mix local{world};
                pool_world(c, local, w);
                const auto record = shape.club_record ? club_at : owner_at;
                const auto list_base = shape.club_record ? record : preferences_at;
                if (!shape.club_record) c.put32(owner_at + 0x18, w.null_preferences ? 0 : preferences_at);
                for (unsigned i = 0; i < 3; ++i) {
                    c.put32(list_base + shape.lists[0] + 4 * i, slots[i]);
                    c.put32(list_base + shape.lists[1] + 4 * i, slots[3 + i]);
                }
                std::uint32_t second = id;
                if (shape.holder_lists) {
                    // The club (argument 2) lists people at +0x5c / +0x68; the person is argument 1.
                    for (unsigned i = 0; i < 3; ++i) c.put32(club_at + 0x5c + 4 * i, holder[i]);
                    for (unsigned i = 0; i < 3; ++i) c.put32(club_at + 0x68 + 4 * i, holder[(i + 1) % 3]);
                }
                if (self && shape.self_check) second = owner_at;
                c.set(3, null_owner ? 0 : record);
                c.set(4, second);
                c.set(5, flag);
            }, n % 4 == 0));
        }
    }
}

// Attribute copies: a person record and an array of 4-byte slots filled with random bytes (the
// original's sign extensions and byte truncations see every value), and each combination of the
// optional outputs.
void add_attribute_copies(std::vector<Fixture>& out) {
    constexpr std::uint32_t person = data + 0x400, other = data + 0x800, array = data + 0x1000,
                            outputs = data + 0x300;
    Mix mix{0xa77b};
    const auto fill = [](Cpu& c, std::uint32_t world) {
        Mix local{world};
        for (std::uint32_t i = 0; i < 0x100; ++i) {
            c.put8(person + i, local.below(3) == 0 ? local.pick<std::uint32_t>({0, 0x7f, 0x80, 0xff}) : local.below(256));
            c.put8(other + i, local.below(256));
        }
        for (std::uint32_t i = 0; i < 0x100; ++i) c.put32(array + 4 * i, local.next() * 2654435761u);
    };
    for (unsigned n = 0; n < 60; ++n) {
        const auto world = mix.next();
        const bool overflow = n % 3 == 0;
        for (const bool with_count : {false, true})
            out.push_back(make(0x100c6150, "attributes to slots", {}, [=](Cpu& c) {
                fill(c, world);
                c.set(3, person);
                c.set(5, array);
                c.set(6, with_count ? outputs : 0);
            }, overflow));
        out.push_back(make(0x100c63c0, "attributes from slots", {}, [=](Cpu& c) {
            fill(c, world);
            c.set(3, person);
            c.set(5, array);
        }, overflow));
        out.push_back(make(0x100c65d0, "all attributes from slots", {}, [=](Cpu& c) {
            fill(c, world);
            c.set(3, person);
            c.set(4, other);
            c.set(5, array);
        }, overflow));
        for (unsigned outputs_present = 0; outputs_present < 8; ++outputs_present)
            out.push_back(make(0x100c5ab0, "all attributes to slots outputs " + std::to_string(outputs_present), {},
                               [=](Cpu& c) {
                fill(c, world);
                c.set(3, person);
                c.set(4, other);
                c.set(5, array);
                c.set(6, outputs_present & 1 ? outputs : 0);
                c.set(7, outputs_present & 2 ? outputs + 4 : 0);
                c.set(8, outputs_present & 4 ? outputs + 8 : 0);
            }, overflow));
    }
}

// Attribute distances: two people with attribute bytes mostly in the 1-20 range (some outside, some 20),
// the static slot arrays and the two flag bytes in each of their four states.
void add_distances(std::vector<Fixture>& out) {
    constexpr std::uint32_t a = data + 0x400, b = data + 0x500, a_other = data + 0x600, b_other = data + 0x680,
                            first_array = data + 0x3000, second_array = data + 0x3400;
    Mix mix{0xd157a};
    for (unsigned n = 0; n < 300; ++n) {
        const auto world = mix.next();
        const bool large = n % 2 == 1;
        const auto states = n / 2 % 4;  // seen / fill flag bytes
        const auto call = [=](Cpu& c) {
            Mix local{world};
            const auto byte = [&] {
                const auto roll = local.below(10);
                if (roll < 6) return local.below(23);
                if (roll == 6) return 20u;
                return local.below(256);
            };
            for (std::uint32_t i = 0; i < 0x100; ++i) {
                c.put8(a + i, byte());
                c.put8(b + i, local.below(4) == 0 ? c.u8(a + i) : byte());
                c.put8(a_other + i, byte());
                c.put8(b_other + i, byte());
            }
            if (local.below(8) == 0) for (const auto person : {a, b}) c.put8(person + 0x0f, 20);
            const std::uint32_t fields[] = {0, 1, 0x7fff, 0x8000, 0xffff, 100, 0xff9c};
            c.put16(a + 0x24, fields[local.below(7)]);
            c.put16(b + 0x24, fields[local.below(7)]);
            c.put16(a + 4, fields[local.below(7)]);
            c.put16(b + 4, fields[local.below(7)]);
            for (std::uint32_t i = 0; i < 70; ++i)
                for (const auto array : {first_array, second_array}) c.put32(array + 4 * i, 0xff81ff81);
            if (large) {
                c.put32(c.r(2) - 0x564c, first_array);
                c.put32(c.r(2) - 0x5648, second_array);
                c.put8(c.r(2) - 0x39eb, states & 1);
                c.put8(c.r(2) - 0x39ea, states >> 1);
                c.set(3, a);
                c.set(4, a_other);
                c.set(5, b);
                c.set(6, b_other);
            } else {
                c.put32(c.r(2) - 0x5654, first_array);
                c.put32(c.r(2) - 0x5650, second_array);
                c.put8(c.r(2) - 0x39ed, states & 1);
                c.put8(c.r(2) - 0x39ec, states >> 1);
                c.set(3, a);
                c.set(5, b);
                c.set(6, local.next());
            }
        };
        out.push_back(make(large ? 0x100c5630 : 0x100c58c0, "distance flags " + std::to_string(states), {}, call, n % 3 == 0));
    }
}

// The category-driven name picker (0x100c4cf0): nation rows, names, a table of clubs (0x128 bytes
// each: nation at +0x68, category byte at +0x119) and the index words the category switch reads.
constexpr std::int32_t category_slots[] = {-0x7584, -0x7588, -0x74d8, -0x74e0, -0x7a28, -0x7a10, -0x7594, -0x7a00,
                                           -0x7af4, -0x7598, -0x759c, -0x75a0, -0x7a14, -0x7a24, -0x75a8, -0x79fc,
                                           -0x75ac, -0x7504, -0x7a18, -0x75b0, -0x75b4, -0x75b8, -0x75bc, -0x75c0,
                                           -0x7514, -0x75c4, -0x750c, -0x74e8, -0x75a4};
void add_category_picker(std::vector<Fixture>& out, std::uint32_t entry) {
    constexpr std::uint32_t person = data + 0x400, club_table = data + 0x10000, index_cells = data + 0x1400,
                            club_count_cell = data + 0x1300, club_table_cell = data + 0x1304;
    constexpr unsigned clubs = 10, nations = 8;
    Mix mix{0xca7e60};
    constexpr std::array<std::int32_t, 10> counts{0, 3, 5, 6, 10, 20, 50, 100, 101, 200};
    for (unsigned n = 0; n < 900; ++n) {
        auto draws = Draws{};
        if (mix.below(5) == 0) {
            draws.seed = mix.next() * 40503u;
        } else {
            for (unsigned i = 0; i < 70; ++i)
                draws.values.push_back(mix.below(3) == 0 ? std::array<std::uint32_t, 8>{0, 1, 2, 16383, 16384, 32766, 32767, 9999}[mix.below(8)] : mix.below(32768));
        }
        const std::int32_t category = n < 66 * 3 ? std::int32_t(n % 66) - 4 : std::int32_t(mix.below(38)) - 3;
        const bool null_person = n % 50 == 7;
        const auto world = mix.next();
        const bool small_bound = mix.below(6) == 0 && draws.seed == 0;
        // A bound above 0x7fff needs samples that keep the name index small.
        if (small_bound && !draws.values.empty()) for (auto& v : draws.values) v &= 3;
        out.push_back(make(entry, "category picker category " + std::to_string(category) +
                                           (null_person ? " null person" : "") + (small_bound ? " big bound" : ""),
                           draws, [=](Cpu& c) {
            Mix local{world};
            c.put32(c.r(2) - 0x7580, data + 0x100);
            c.put32(data + 0x100, rows);
            const auto bound = small_bound ? 0x9000 : local.below(400);
            for (const std::int32_t slot : {-0x7550, -0x754c}) c.put32(c.r(2) + std::uint32_t(slot), data + 0x110);
            c.put32(data + 0x110, bound);
            c.put32(c.r(2) - 0x752c, data + 0x120);
            c.put32(c.r(2) - 0x7530, data + 0x128);
            c.put32(data + 0x128, names);
            c.put32(data + 0x120, names);
            c.put32(c.r(2) - 0x7b04, club_count_cell);
            c.put32(club_count_cell, local.below(3) == 0 ? clubs : 1 + local.below(clubs));
            c.put32(c.r(2) - 0x7b98, club_table_cell);
            c.put32(club_table_cell, club_table);
            c.put32(c.r(2) - 0x7590, data + 0x1380);
            c.put32(c.r(2) - 0x758c, data + 0x1384);
            c.put32(data + 0x1380, local.below(clubs));
            c.put32(data + 0x1384, local.below(clubs));
            unsigned k = 0;
            for (const auto slot : category_slots) {
                c.put32(c.r(2) + std::uint32_t(slot), index_cells + 4 * k);
                c.put32(index_cells + 4 * k, local.below(clubs));
                ++k;
            }
            // Slots the switch reads through (-0x7584 and -0x7588 are among category_slots).
            for (unsigned nation = 0; nation < nations; ++nation) {
                c.put32(rows + nation * 0x24 + 0, local.below(20));
                c.put32(rows + nation * 0x24 + 4, local.below(20));
                c.put32(rows + nation * 0x24 + 0x18, counts[local.below(counts.size())]);
                c.put32(rows + nation * 0x24 + 0x1c, counts[local.below(counts.size())]);
            }
            for (std::uint32_t i = 0; i < 500; ++i) {
                c.put8(names + 0x2c * i + 0x28, local.below(256));
                // 0x100c4b00 (real) reads the record's first bytes as text: short, with or without a dot.
                const auto length = local.below(6);
                for (std::uint32_t k = 0; k < length; ++k) c.put8(names + 0x2c * i + k, local.below(5) == 0 ? '.' : 'a' + local.below(5));
                c.put8(names + 0x2c * i + length, 0);
            }
            for (std::uint32_t i = 1; i <= 100; ++i) c.put8(names - 0x2c * i + 0x28, local.below(256));
            for (unsigned i = 0; i < clubs; ++i) {
                c.put32(club_table + i * 0x128 + 0x68, local.below(nations));
                c.put8(club_table + i * 0x128 + 0x119, local.below(2) ? std::uint32_t(category) : local.below(40));
            }
            c.put32(person + 0x68, local.below(nations));
            c.put8(person + 0x119, std::uint32_t(category));
            c.set(3, null_person ? 0 : person);
        }, n % 7 == 0));
    }
}

// The attribute balance (0x100c3870): a person whose record at +0x1c holds attribute bytes of mixed
// size (some beyond the widening limit, some negative), with bytes +0x60/+0x62 equal to 10 or not.
void add_balance(std::vector<Fixture>& out) {
    constexpr std::uint32_t person = data + 0x400, record = data + 0x800;
    Mix mix{0xba1a};
    for (unsigned n = 0; n < 400; ++n) {
        const auto world = mix.next();
        const unsigned shape = n % 40 == 1 ? 1 : n % 40 == 2 ? 2 : 0;  // null person, null record
        const unsigned scale = n % 5;                                    // how far the bytes spread
        out.push_back(make(0x100c3870, "balance scale " + std::to_string(scale) + " shape " + std::to_string(shape), {},
                           [=](Cpu& c) {
            Mix local{world};
            for (std::uint32_t i = 0; i < 0x100; ++i) {
                const auto roll = local.below(10);
                const std::uint32_t spread[] = {8, 20, 40, 100, 256};
                auto value = roll < 7 ? std::uint32_t(std::int32_t(local.below(spread[scale]) - spread[scale] / 2)) & 0xff
                                      : local.below(256);
                if (value == 0x81) value = 0x80;  // -127 ends the slot array: fewer than six slots would never settle
                c.put8(record + i, value);
            }
            for (std::uint32_t i = 0; i < 0x100; ++i) c.put8(person + i, local.below(256));
            c.put32(person + 0x1c, shape == 2 ? 0 : record);
            c.put8(person + 0x60, local.below(3) == 0 ? 10 : local.below(256));
            c.put8(person + 0x62, local.below(3) == 0 ? 10 : local.below(256));
            c.set(3, shape == 1 ? 0 : person);
        }, n % 3 == 0));
    }
}

// 0x100c53e0: a table of 0x2c-byte records, the static flag and cached index, the record count (zero, one,
// positive, negative within +-65535, and below -65535 for the floating-point path) and the sample pool.
void add_unmarked_record(std::vector<Fixture>& out) {
    constexpr std::uint32_t table = data + 0x8000;
    Mix mix{0x53e0};
    for (unsigned n = 0; n < 500; ++n) {
        const auto world = mix.next();
        const auto draws = script(mix, 1);
        const unsigned kind = n % 10;
        // The records' count: the wide path needs a negative count beyond -0xffff.
        const std::int32_t counts[] = {0, 1, 2, 30, 100, -1, -7, -0xffff, -0x10000, -0x10001};
        const auto count = counts[kind];
        const bool zero_first = mix.below(2) == 0;      // some record starts with a zero byte
        const unsigned states = mix.below(4);           // seen flag, cached index
        out.push_back(make(0x100c53e0, "unmarked record count " + std::to_string(count), draws, [=](Cpu& c) {
            Mix local{world};
            ListWorld w{local.below(4), local.below(3) == 0 ? 0xfffffff0u : local.below(0x8000), false};
            pool_world(c, local, w);
            c.put32(c.r(2) - 0x7554, data + 0x100);
            c.put32(data + 0x100, std::uint32_t(count));
            c.put32(c.r(2) - 0x7534, data + 0x104);
            // For counts below -0xffff the floating-point path indexes backwards from the table; keep it mapped.
            const auto base = count < -0xffff ? table + 0x2c * 0x10100u : table;
            c.put32(data + 0x104, base);
            for (std::uint32_t i = 0; i < 100; ++i) c.put8(table + 0x2c * i, 1 + local.below(255));
            if (zero_first && count > 0) c.put8(table + 0x2c * local.below(std::uint32_t(count < 100 ? count : 100)), 0);
            c.put8(c.r(2) - 0x39e9, states & 1);
            c.put32(c.r(2) - 0x39e8, (states >> 1) ? local.below(20) : 0xffffffffu);
        }, n % 3 == 0));
    }
}

// range_sample (0x100a0d80): bounds from 0 through the 16-bit limits to beyond 2^31 / 10 (the floating-point
// levels; the original's fifth level, a real self-call, needs a bound over 2^64 and never runs), the pool
// cursor at each of its positions (a wrap draws), salts including negative sums, scripted and real draws.
void add_range_sample(std::vector<Fixture>& out) {
    Mix mix{0x5a3b1e};
    constexpr std::int32_t bounds[] = {0, 1, -1, 2, 7, -7, 100, -100, 32767, 65534, 65535, -65535, 65536, -65536, 65537,
                                       -65537, 100000, -100000, 655350, 5000000, -5000000, 100000000, 0x7fffffff,
                                       -0x7fffffff, std::int32_t(0x80000000), 1000000, 4294, 21474836, 429496730, -429496730, 858993460, 1288490190, 214748365, -214748365};
    for (unsigned n = 0; n < 900; ++n) {
        const auto bound = bounds[n % (sizeof bounds / sizeof bounds[0])];
        const auto draws = script(mix, 6);
        const auto world = mix.next();
        out.push_back(make(0x100a0d80, "range sample bound " + std::to_string(bound), draws, [=](Cpu& c) {
            Mix local{world};
            constexpr std::uint32_t salts[] = {0, 1, 0x7fff, 0xffff, 0x80000000u, 0xfffffff0u, 0xfffffffeu, 12345};
            ListWorld w{local.below(4), salts[local.below(8)], false};
            pool_world(c, local, w);
            c.set(3, std::uint32_t(bound));
        }, n % 3 == 0));
    }
}

// 0x100d3570 and 0x100da5b0: a person with random bytes whose pointer fields (+0x14 player, +0x18 preferences)
// point at records of random bytes; the contract manager's lookup (0x10082f90) is scripted.
void add_value_draws(std::vector<Fixture>& out) {
    constexpr std::uint32_t person = data + 0x400, player = data + 0x600, preferences = data + 0x800,
                            manager_word = data + 0xb00, contract = data + 0xc00;
    Mix mix{0xd3570};
    for (unsigned n = 0; n < 1200; ++n) {
        const bool second = n % 2 == 1;
        const auto draws = script(mix, 12);
        const auto world = mix.next();
        const auto contract_answer = mix.below(3) == 0 ? 0u : contract;
        const auto byte_arg = mix.below(256), third = mix.below(4) == 0 ? 0u : mix.below(256);
        const bool null_person = n % 40 == 5;
        out.push_back(make(second ? 0x100da5b0 : 0x100d3570, std::string(second ? "generate value" : "adjust by draws") +
                                                                   (null_person ? " null person" : ""), draws, [=](Cpu& c) {
            Mix local{world};
            ListWorld w{local.below(4), local.below(3) == 0 ? 0xfffffff0u : local.below(0x8000), false};
            pool_world(c, local, w);
            for (std::uint32_t i = 0; i < 0x100; ++i) {
                const auto roll = local.below(4);
                c.put8(person + i, roll == 0 ? local.pick<std::uint32_t>({0, 1, 0x7f, 0x80, 0xff}) : roll == 1 ? local.below(256) : local.below(40));
                c.put8(player + i, local.below(4) == 0 ? local.below(256) : local.below(40));
                c.put8(preferences + i, local.below(4) == 0 ? local.below(256) : local.below(40));
                c.put8(contract + i, local.below(256));
            }
            c.put32(person + 0x14, player);
            c.put32(person + 0x18, preferences);
            c.put32(c.r(2) - 0x7b7c, manager_word);
            c.put32(manager_word, data + 0xb40);
            c.set(3, null_person ? 0 : person);
            c.set(4, byte_arg);
            c.set(5, third);
        }, n % 3 == 0, [=](State& s) {
            if (s.pc != 0x10082f90) return false;
            Cpu c(s);
            record_arguments(c, 3);
            c.set(3, contract_answer);
            s.pc = c.lr();
            return true;
        }));
    }
}
}

std::vector<Fixture> generation_fixtures() {
    std::vector<Fixture> found;
    add_name_pickers(found);
    add_list_routines(found);
    add_attribute_copies(found);
    add_distances(found);
    add_balance(found);
    add_unmarked_record(found);
    add_range_sample(found);
    add_value_draws(found);
    add_category_picker(found, 0x100c4cf0);
    add_category_picker(found, 0x100c4400);
    return found;
}
}
