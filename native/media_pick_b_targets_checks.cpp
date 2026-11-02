// Cases for 0x10699d10 weigh_story_targets: a scratch club whose staff list holds scratch persons
// (the squad copy is steered by answering the filter 0x100ce320 with 0), a scratch subject person, real
// clubs as the rivals, and a scripted random table. Every callee outside the routine is answered, one
// result per case; the persons' bytes, the callees' answers and the table vary by case from a hash, so the
// 57 members of each list walk through the weight ladders and both draws.
#include "media_checks.hpp"

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
constexpr std::uint32_t entry_weigh = 0x10699d10;
constexpr std::uint32_t club_at = scratch_base + 0x1000, parent_at = scratch_base + 0x1400, standing_at = scratch_base + 0x1a00,
    subject_at = scratch_base + 0x1800, rivals_at = scratch_base + 0x1c00, table_at = scratch_base + 0x8000,
    persons_at = scratch_base + 0x20000, contracts_at = scratch_base + 0x30000, links_at = scratch_base + 0x38000,
    ptr18_at = scratch_base + 0x1500;
constexpr std::uint32_t id_a = 0xa001, id_b = 0xb002, id_club = 0x1234, parent_word = 0xc0de;

std::uint32_t mix(std::uint32_t a, std::uint32_t b) {
    std::uint32_t h = a * 0x9e3779b1u ^ (b + 0x7f4a7c15u) * 0x85ebca6bu;
    h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
    return h;
}
template <class T> T pick(std::uint32_t h, std::initializer_list<T> values) { return *(values.begin() + h % values.size()); }
void put32(Writes& w, std::uint32_t at, std::uint32_t v) { w.push_back({4, at, v}); }
void put16(Writes& w, std::uint32_t at, std::uint32_t v) { w.push_back({2, at, v}); }
void put8(Writes& w, std::uint32_t at, std::uint32_t v) { w.push_back({1, at, v}); }
}

MEDIA_CASES("pick_b_targets") {
    const auto slot_a = scenario.toc(-0x7a04), slot_b = scenario.toc(-0x7a08), slot_parent = scenario.toc(-0x7a0c);
    const auto cursor = scenario.toc(-0x7b84), end = scenario.toc(-0x7b88), start = scenario.toc(-0x7b8c), seed = scenario.toc(-0x7b90);
    const auto real_club = [&](unsigned n) { return scenario.club(1 + n); };

    const auto base_registers = [&](std::uint32_t kind, std::uint32_t person, std::uint32_t club, std::uint32_t rivals, std::uint32_t count) {
        return Registers{{3, 0x31313131}, {4, kind}, {5, person}, {6, club}, {7, rivals}, {8, count}, {9, 0x91919191}, {10, 0xa1a1a1a1}};
    };
    // The random table: `entries` samples from table_at on, the cursor at its start (or `back` entries from the end).
    const auto table = [&](std::uint32_t salt, unsigned entries, std::uint32_t global_seed, int back, std::uint32_t modulus) {
        Writes w;
        for (unsigned i = 0; i < entries; ++i) w.push_back({2, table_at + 2 * (i + 1), mix(salt, i) % modulus});
        const auto last = table_at + 2 * entries;
        put32(w, cursor, back ? last - 2 * std::uint32_t(back) : table_at);
        put32(w, end, last);
        put32(w, start, table_at);
        put32(w, seed, global_seed);
        return w;
    };
    const auto answers_for = [&](std::uint32_t h, bool standing_present) {
        const auto verdict = [&](std::uint32_t salt) { return pick<std::uint32_t>(mix(h, salt), {0, 1, 1, 0x100, 0x80, 0xff, 0}); };
        std::vector<std::pair<std::uint32_t, std::uint32_t>> a;
        a.push_back({0x100ce320, 0});  // the squad filter: 0 copies the club's list whole
        const auto tie = mix(h, 1) % 4;  // the near-a / near-b answers together
        a.push_back({0x100d9480, tie == 0 ? 1u : tie == 3 ? 0x80u : 0u});
        a.push_back({0x100d94d0, tie == 1 ? 1u : tie == 3 ? 0x100u : 0u});
        a.push_back({0x10464160, verdict(2)});
        a.push_back({0x100d97a0, 0});
        a.push_back({0x100d3150, pick<std::uint32_t>(mix(h, 3), {0, 0, 1, 0x180})});
        a.push_back({0x10082f90, standing_present ? standing_at : 0u});
        a.push_back({0x10380710, verdict(4)});
        a.push_back({0x1014c2e0, pick<std::uint32_t>(mix(h, 5), {0, 0, 1})});
        a.push_back({0x100c7a20, verdict(6)});
        a.push_back({0x100d3300, verdict(7)});
        a.push_back({0x100d9520, pick<std::uint32_t>(mix(h, 8), {0, 0, 0, 1})});
        a.push_back({0x100d9580, pick<std::uint32_t>(mix(h, 9), {0, 1, 1})});
        a.push_back({0x100d9b80, 0});
        // 0x1037fa80 and 0x1037f8a0 run for real on the scratch standing record: the original's calls to them are direct.
        return a;
    };

    // The world of one case: globals, the club and its staff, the subject, the rivals, the standing record.
    // `rivals_count` entries of real clubs get a configuration each; staff members vary by index and salt.
    const auto world = [&](std::uint32_t h, unsigned club_class, unsigned rivals_count, bool subject_listed, bool manager, std::uint32_t morale) {
        Writes w;
        // The slots hold addresses of the words the routine compares.
        put32(w, slot_a, id_a);
        put32(w, slot_b, id_b);
        put32(w, slot_parent, parent_word);
        const std::uint32_t id = club_class == 0 ? id_a : club_class == 1 ? id_b : id_club;
        put32(w, club_at, id);
        const auto parent = mix(h, 11) % 3;  // 0 none, 1 a parent whose id word matches the global, 2 one that doesn't
        put32(w, club_at + 0x30, parent == 0 ? 0 : parent_at);
        put32(w, parent_at, parent == 1 ? parent_word : 0x1f1f);
        // Persons.
        const auto person_of = [&](unsigned j) { return persons_at + j * 0x80; };
        const auto slot_pointer = [&](unsigned j) -> std::uint32_t {
            const auto r = mix(h, 100 + j) % 40;
            if (r == 0 || r == 1) return 0;
            if (subject_listed && r == 2) return subject_at;
            return person_of(j);
        };
        for (unsigned j = 0; j < 50; ++j) put32(w, club_at + 0x98 + 4 * j, slot_pointer(j));
        put32(w, club_at + 0x80, slot_pointer(50));
        put32(w, club_at + 0x90, manager ? person_of(51) : 0);
        put32(w, club_at + 0x94, slot_pointer(52));
        for (unsigned k = 0; k < 5; ++k) put32(w, club_at + 0x160 + 4 * k, slot_pointer(53 + k));
        if (manager) put8(w, person_of(51) + 0x62, mix(h, 12) % 3);
        for (unsigned j = 0; j < 58; ++j) {
            const auto p = person_of(j), contract = contracts_at + j * 0x80, link = links_at + j * 0x80;
            const auto f = [&](std::uint32_t salt) { return mix(h, j * 64 + salt); };
            put32(w, p + 0x14, f(1) % 11 == 0 ? 0 : contract);
            put32(w, p + 0x18, f(2) % 13 == 0 ? 0 : ptr18_at);
            put32(w, p + 0x1c, f(3) % 6 == 0 ? link : 0);
            put16(w, contract + 0xa, pick<std::uint32_t>(f(4), {0x1000, 0x1099, 0x109a, 0x109b, 0x1675, 0x1676, 0x1677, 0x2000, 0x8001, 0xffff, 0x0}));
            put8(w, contract + 0x3d, pick<std::uint32_t>(f(5), {0, 1, 7, 0x20, 0xff}));
            put8(w, link + 0x3d, pick<std::uint32_t>(f(6), {0, 3, 9, 0x40, 0xfe}));
            put16(w, p + 0x5c, pick<std::uint32_t>(f(7), {0x10, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x20, 0x8010, 0x1180, 0x1117, 0x1114}));
            put8(w, p + 0x65, pick<std::uint32_t>(f(8), {0, 3, 4, 5, 6, 0xf, 0x10, 30, 0x80, 0xf0}));
            put8(w, p + 0x66, pick<std::uint32_t>(f(9), {0, 5, 6, 0xe, 0xf, 0x10, 0x81}));
            put8(w, p + 0x67, pick<std::uint32_t>(f(10), {0, 1, 1, 2, 5, 6, 0xa, 0x14, 0x14, 0x80, 0xff}));
            put8(w, p + 0x69, pick<std::uint32_t>(f(11), {0, 3, 5, 6, 7, 30, 0xff}));
        }
        // The subject.
        put16(w, subject_at + 0x5c, pick<std::uint32_t>(mix(h, 20), {0x10, 0x14, 0x15, 0x30, 0x1180, 0xff, 0x16}));
        put8(w, subject_at + 0x67, pick<std::uint32_t>(mix(h, 21), {0, 5, 0x19, 0x20, 0xff}));
        put8(w, standing_at + 0x44, morale);
        // The rivals: real clubs 1.., each configured by its index.
        for (unsigned j = 0; j < rivals_count; ++j) {
            const auto other = real_club(j);
            put32(w, rivals_at + 4 * j, 1 + j);
            const auto r = mix(h, 200 + j);
            const auto klass = (j + r) % 3;
            put32(w, other, klass == 0 ? id_a : klass == 1 ? id_b : 0x5555);
            const auto link = (j / 3 + r / 3) % 4;
            put32(w, other + 0x74, link == 1 ? club_at : 0);
            put32(w, other + 0x78, link == 2 ? club_at : 0);
            put32(w, other + 0x7c, link == 3 ? club_at : 0);
            const auto shared = (j / 12 + r / 12) % 3;
            put32(w, other + 0x30, shared == 0 ? parent_at : shared == 1 ? parent_at + 0x100 : 0);
        }
        return w;
    };
    const auto join = [](Writes a, const Writes& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    const auto add = [&](Registers registers, Writes memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, const char* label) {
        cases.push_back({.entry = entry_weigh, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = label,
                         .answers = std::move(answers)});
    };

    // Guards: no club, no person.
    add(base_registers(0x20, subject_at, 0, rivals_at, 3), {}, answers_for(1, true), "no club");
    add(base_registers(0x20, 0, club_at, rivals_at, 3), world(1, 2, 3, false, true, 40), answers_for(1, true), "no person");
    // A scored sweep: kinds, club classes, rival lists, answers, tables.
    const std::uint32_t kinds[] = {0x20, 0x1e, 5, 0x10020, 0x18020, 0x1001e, 0x21, 0xffff801e};
    for (unsigned n = 0; n < 520; ++n) {
        const auto h = mix(n, 77);
        const auto kind = n < 64 ? kinds[n % 8] : kinds[h % 8];
        const auto club_class = mix(h, 1) % 3;
        const auto rivals_mode = mix(h, 2) % 6;  // 0 none, 1 count 0, 2 negative count, 3 six, 4 all 48, 5 twenty
        const unsigned rivals_count = rivals_mode == 3 ? 6 : rivals_mode == 4 ? 48 : rivals_mode == 5 ? 20 : 0;
        const std::uint32_t list_ptr = rivals_mode == 0 ? 0 : rivals_at;
        const std::uint32_t count = rivals_mode == 2 ? 0xffu : rivals_mode == 0 ? 5u : rivals_count;
        const auto morale = pick<std::uint32_t>(mix(h, 3), {0, 9, 10, 19, 20, 0x64, 0x7f, 0x80, 0xc8, 0xff, 0x81});
        const bool standing = mix(h, 4) % 5 != 0;
        const auto memory = join(world(h, club_class, rivals_count, mix(h, 5) % 2, mix(h, 6) % 4 != 0, morale),
                                 table(mix(h, 7), 480, pick<std::uint32_t>(mix(h, 8), {0, 0, 0x1fff, 0x80000001u}), 0, pick<std::uint32_t>(mix(h, 9), {65536, 65536, 200, 30})));
        add(base_registers(kind, subject_at, club_at, list_ptr, count), memory, answers_for(h, standing), ("sweep " + std::to_string(n)).c_str());
    }
    // The table running out: the refill draws a new seed with rand().
    for (unsigned n = 0; n < 40; ++n) {
        const auto h = mix(n, 5000);
        const auto memory = join(world(h, 2, 6, true, true, 40), table(h, 60, 0x10, 1 + int(n), 65536));
        add(base_registers(n % 2 ? 0x20 : 5, subject_at, club_at, rivals_at, 6), memory, answers_for(h, true), "refill");
    }
    // A real club with real persons and rivals, nothing answered but the filter: the callees' own bodies run.
    for (unsigned n = 0; n < 4; ++n)
        add(base_registers(0x20, scenario.global(-0x7be0) + (n + 5) * 0x70, scenario.club(n + 3), rivals_at, 3),
            join(world(n, 2, 3, false, true, 40), table(n + 90, 480, 0, 0, 65536)), {}, "real");
}
