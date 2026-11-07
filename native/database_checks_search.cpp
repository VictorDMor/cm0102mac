// cm-database-checks fixtures for DATABASE.CPP's search block (database_checks.hpp).
// The sorters run the real qsort and the real comparators of the game (so the
// comparator call order and tie behaviour are compared); library calls that
// need state a fixture cannot build are stubbed with record_call, which keeps
// their arguments and the registers a callee would save.
#include "database_checks.hpp"
#include "database_records_ce320.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <span>
#include <bit>
#include <functional>
#include <limits>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;

constexpr std::uint32_t club_table_word = data + 0x100, club_count_word = data + 0x104, id_array_word = data + 0x108,
                        ranked_table_word = data + 0x10c, ranked_count_word = data + 0x110,
                        ranked_floor_word = data + 0x114, ranked_tail_word = data + 0x118,
                        flag_word = data + 0x11c, target_word = data + 0x120, buffer_word = data + 0x124,
                        clubs = data + 0x2000, id_array = data + 0x4000, ranked = data + 0x5000,
                        ranked_alt = data + 0x6000, links = data + 0xc000, byte_table = data + 0x8000,
                        pool = data + 0x9000, record = data + 0xa000, subject = data + 0xa400;
constexpr std::array assert_calls{0x10739890u, 0x10768810u, 0x1077b720u, 0x10769b60u};

void put_toc(Cpu& c, std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); }

std::vector<Fixture> fixtures;
void add(std::uint32_t entry, std::string text, std::function<void(Cpu&)> setup, bool overflow = false,
         std::function<bool(State&)> stub = nullptr) {
    Fixture f;
    f.entry = entry;
    f.description = std::move(text);
    f.setup = std::move(setup);
    f.stub = std::move(stub);
    f.summary_overflow = overflow;
    fixtures.push_back(std::move(f));
}
std::string hex(std::uint32_t value) { return hex_address(value); }

// The game's comparators are scripted (other blocks may have rewritten them,
// and what is checked here is the sort's call order and its results): each
// call is recorded with its arguments and answers from the items' words.
// The answer is the sign of the difference of the keys, so ties answer 0.
std::int32_t sign(std::int64_t value) { return value < 0 ? -1 : value > 0 ? 1 : 0; }
bool comparator_stub(State& s) {
    Cpu c(s);
    std::int32_t answer = 0;
    switch (s.pc) {
    case 0x100cd390:  // club ids: by id word
        answer = sign(std::int64_t(c.i32(c.r(3))) - c.i32(c.r(4)));
        break;
    case 0x100cde70:  // 0x1c-byte entries: by the key of the linked record, then the halfword
        answer = sign(std::int64_t(c.i32(c.u32(c.r(3) + 4))) - c.i32(c.u32(c.r(4) + 4)));
        if (answer == 0) answer = sign(std::int64_t(c.i16(c.r(3) + 0x18)) - c.i16(c.r(4) + 0x18));
        break;
    case 0x100d5ce0:  // the byte table: by (byte * selector) mod 7, ties answer 0
        {
            const auto selector = std::int32_t(c.u8(c.r(2) - 0x39ee)) | 1;
            answer = sign((c.u8(c.r(3)) * selector) % 7 - (c.u8(c.r(4)) * selector) % 7);
        }
        break;
    default:
        return false;
    }
    record_call(c);
    c.set(3, std::uint32_t(answer));
    s.pc = c.lr();
    return true;
}

// 0x100cae30: refill flag, club count (including none and negative) and
// ratings that give ties and every order of the comparator.
void club_id_sorter() {
    constexpr std::array<std::uint32_t, 6> counts{0, 1, 2, 5, 6, 3};
    for (const std::uint32_t r3 : {0u, 1u, 0xffffff80u, 0x100u, 0x12345600u, 0x7fu})
        for (const auto count : counts)
            for (const bool ties : {false, true})
                for (const bool overflow : {false, true}) {
                    add(0x100cae30, "refill r3=" + hex(r3) + " count=" + hex(count) + " ties=" + std::to_string(ties),
                         [=](Cpu& c) {
                             put_toc(c, -0x7be8, club_table_word);
                             put_toc(c, -0x7b50, club_count_word);
                             put_toc(c, -0x765c, id_array_word);
                             c.put32(club_table_word, clubs);
                             c.put32(club_count_word, count);
                             c.put32(id_array_word, id_array);
                             const std::array<std::uint32_t, 6> ids = ties ? std::array<std::uint32_t, 6>{3, 3, 1, 1, 0xffffffff, 3} : std::array<std::uint32_t, 6>{3, 0xffffffff, 1, 4, 2, 0};
                             const std::array<std::uint16_t, 6> ratings{50, 0x8001, 50, 7, std::uint16_t(ties ? 50 : 90), 0xffff};
                             for (unsigned i = 0; i < 6; ++i) {
                                 c.put32(clubs + i * 0x248, ids[i]);
                                 c.put16(clubs + i * 0x248 + 0x1ec, ratings[i]);
                                 c.put32(id_array + i * 4, 5 - i);  // stale contents when not refilled
                             }
                             c.set(3, r3);
                             c.set(9, 0x55aa);
                         },
                         overflow, comparator_stub);
                }
}

// 0x100d5d70: the selector byte (the comparator orders by it).
void byte_sorter() {
    for (const std::uint32_t r3 : {0u, 1u, 2u, 3u, 7u, 0x80u, 0x1ffu, 0xffffffffu}) {
        add(0x100d5d70, "selector " + hex(r3), [=](Cpu& c) {
                 put_toc(c, -0x56b0, byte_table);
                 c.set(3, r3);
                 c.set(7, 0x7777);
             }, r3 == 3, comparator_stub);
    }
}

// 0x100c4180: entries kept or dropped by every test, the realloc answer and
// the floor word (including past the count, which makes the span negative).
void ranked_compactor() {
    // Entry kinds: 0 no link; 1 link without inner record; 2 link with inner record.
    struct Layout { std::vector<int> kinds; std::vector<std::uint8_t> bytes; };
    const std::vector<Layout> layouts{
        {{}, {}},
        {{0}, {0}},
        {{1}, {0}}, {{1}, {1}}, {{1}, {0x80}}, {{2}, {0}}, {{2}, {0xff}}, {{2}, {1}},
        {{1, 1, 1, 1}, {1, 1, 1, 1}},
        {{0, 1, 2, 2, 1}, {1, 0, 0x80, 1, 3}},
        {{2, 2, 2, 2, 2}, {1, 2, 3, 4, 0}},
        {{1, 2, 1, 2, 0, 2}, {5, 0, 0, 7, 1, 1}}};
    for (const auto& layout : layouts)
        for (const std::uint32_t floor : {0u, 1u, 3u, 0xfffffffeu})
            for (const bool moved : {false, true})
                for (const bool overflow : {false, true}) {
                    add(0x100c4180, "entries " + std::to_string(layout.kinds.size()) + " first kind " +
                                        std::to_string(layout.kinds.empty() ? -1 : layout.kinds[0]) + " floor " + hex(floor) +
                                        " moved " + std::to_string(moved),
                        [=](Cpu& c) {
                            put_toc(c, -0x7a34, ranked_table_word);
                            put_toc(c, -0x7a38, ranked_count_word);
                            put_toc(c, -0x751c, ranked_floor_word);
                            put_toc(c, -0x7518, ranked_tail_word);
                            c.put32(ranked_table_word, ranked);
                            c.put32(ranked_count_word, std::uint32_t(layout.kinds.size()));
                            c.put32(ranked_floor_word, floor);
                            for (unsigned i = 0; i < 8; ++i) {
                                const auto entry = ranked + i * 0x1c;
                                const auto kind = i < layout.kinds.size() ? layout.kinds[i] : 0;
                                const auto byte = i < layout.bytes.size() ? layout.bytes[i] : std::uint8_t(0);
                                const auto link = links + i * 0x600, inner = link + 0x300;
                                for (unsigned k = 0; k < 7; ++k) c.put32(entry + k * 4, 0x01010101 * (i + 1) + k);
                                c.put16(entry + 0x18, i % 3 == 1 ? 0xffff : 0x8000 + i * 5);
                                c.put32(entry + 4, kind == 0 ? 0 : link);
                                c.put32(link, 20 - (i * 7) % 5);  // the key the comparator orders by
                                c.put32(link + 8, kind == 2 ? inner : 0);
                                c.put8(link + 0x120, kind == 1 ? byte : 0x55);
                                c.put8(inner + 0x120, kind == 2 ? byte : 0x66);
                                c.put32(ranked_alt + i * 0x1c + 4, link);
                                c.put32(ranked_alt + i * 0x1c, 99 - i);
                            }
                            c.put32(ranked_alt + 4, links);
                        },
                        overflow, [=](State& s) {
                            if (comparator_stub(s)) return true;
                            // The allocator's realloc wrapper (0x107751b0) runs for real; its two
                            // callees are scripted: the heap, and the move of the block.
                            if (s.pc != 0x10774b10 && s.pc != 0x10774f40) return false;
                            Cpu c(s);
                            record_call(c);
                            c.set(3, s.pc == 0x10774b10 ? data + 0xe000 : moved ? ranked_alt : c.r(4));
                            s.pc = c.lr();
                            return true;
                        });
                }
}

// 0x100d21f0: argument checks, then the three paths (flag byte clear, lookup
// without a hit, lookup and test). Only the library calls and the lookup
// (0x106fdb00) are scripted: the callees inside DATABASE.CPP run for real on
// both sides (a stub cannot reach a call the generated original makes inside
// the module), so the record the slot is read from is real data.
void search_dispatcher() {
    constexpr std::uint32_t lookup = 0x106fdb00, number_pool = data + 0x3000, linked = data + 0xb400;
    const auto stub = [](std::uint32_t lookup_answer, std::uint32_t fog) {
        return [lookup_answer, fog](State& s) {
            const bool assertion = std::ranges::find(assert_calls, s.pc) != assert_calls.end();
            if (!assertion && s.pc != lookup && s.pc != 0x106fe280) return false;
            Cpu c(s);
            record_call(c);
            c.set(3, assertion ? (s.pc == 0x10768810 ? data + 0xb000 : data + 0xb100)
                               : s.pc == lookup ? lookup_answer : fog);
            s.pc = c.lr();
            return true;
        };
    };
    for (const std::uint32_t r4 : {0u, record, high + 0x100})
        for (const std::uint32_t r5 : {0u, 1u, 2u, 3u, 8u, 0x1fu, 0x20u, 0x80u, 0xffffff81u, 0x101u, 0x7fu, 0xffffffffu, 0x41fu})
            for (const std::uint32_t flag : {0u, 1u, 0x80u, 0x100u})
                for (const std::uint32_t lookup_answer : {0u, data + 0xb600})
                  for (const std::uint32_t fog : {0u, 1u, 2u, 3u, 0x102u})
                    for (const bool overflow : {false, true}) {
                        if (flag != 1 && (lookup_answer != 0 || fog != 0)) continue;
                        if (lookup_answer == 0 && fog != 0) continue;
                        add(0x100d21f0, "r4=" + hex(r4) + " slot=" + hex(r5) + " flag=" + hex(flag) + " lookup=" + hex(lookup_answer) +
                                            " fog=" + hex(fog),
                            [=](Cpu& c) {
                                put_toc(c, -0x797c, flag_word);
                                put_toc(c, -0x7978, target_word);
                                put_toc(c, -0x7ba8, buffer_word);
                                put_toc(c, -0x5618, pool);
                                put_toc(c, -0x5614, number_pool);
                                c.put8(flag_word, flag);
                                c.put32(target_word, data + 0xb200);
                                // The constants of the rating formula (0x100d2e60, 0x100d2d70).
                                for (const auto& [offset, value] : std::array<std::pair<std::uint32_t, double>, 8>{{
                                         {0x18, 4503601774854144.0}, {0x48, 0.5}, {0x60, 0.1}, {0xa0, 2.0}, {0xb0, 10.0},
                                         {0xb8, 20.0}, {0xc0, 3.0}, {0xc8, 1.0}}})
                                    c.put64(number_pool + offset, std::bit_cast<std::uint64_t>(value));
                                for (unsigned i = 0; i < 0x80; ++i) c.put8(linked + i, 0x20 + i * 3);
                                c.put32(record + 0x14, linked);
                                c.put32(high + 0x114, linked);
                                c.set(3, subject);
                                c.set(4, r4);
                                c.set(5, r5);
                            },
                            overflow, stub(lookup_answer, fog));
                    }
}

// 0x100d1d70: the guards, then both modes of the fog answer against every slot
// byte and every residue of the record's first word modulo 4.
void slot_visibility() {
    constexpr std::uint32_t lookup = 0x106fdb00, fog_lookup = 0x106fe280;
    const auto stub = [](std::uint32_t lookup_answer, std::uint32_t fog) {
        return [lookup_answer, fog](State& s) {
            if (s.pc != lookup && s.pc != fog_lookup) return false;
            Cpu c(s);
            record_call(c);
            c.set(3, s.pc == lookup ? lookup_answer : fog);
            s.pc = c.lr();
            return true;
        };
    };
    const auto setup = [](std::uint32_t person_arg, std::uint32_t record_arg, std::uint32_t slot_arg, std::uint32_t target,
                          std::uint32_t flag, std::uint32_t word) {
        return [=](Cpu& c) {
            put_toc(c, -0x7978, target_word);
            put_toc(c, -0x797c, flag_word);
            c.put32(target_word, target);
            c.put8(flag_word, flag);
            c.put32(record, word);
            c.set(3, person_arg);
            c.set(4, record_arg);
            c.set(5, slot_arg);
        };
    };
    // The guards, in each order they can fail.
    for (const std::uint32_t person_arg : {0u, subject, high + 0x40})
        for (const std::uint32_t record_arg : {0u, record, high + 0x80})
            for (const std::uint32_t target : {0u, 0x80000000u})
                for (const std::uint32_t flag : {0u, 1u, 0x80u, 0x100u})
                    for (const std::uint32_t lookup_answer : {0u, data + 0xb600})
                        for (const bool overflow : {false, true}) {
                            if (record_arg == high + 0x80 && person_arg != subject) continue;
                            if (flag == 0x100 && lookup_answer != 0) continue;
                            add(0x100d1d70, "guards person=" + hex(person_arg) + " record=" + hex(record_arg) + " target=" + hex(target) +
                                                " flag=" + hex(flag) + " lookup=" + hex(lookup_answer),
                                [=](Cpu& c) {
                                    setup(person_arg, record_arg, 7, target, flag, 0)(c);
                                    c.put32(high + 0x80, 5);
                                },
                                overflow, stub(lookup_answer, 3));
                        }
    for (const std::uint32_t fog : {0u, 1u, 2u, 3u, 0x102u, 0x1feu})
        for (const std::uint32_t word : {0u, 1u, 2u, 3u, 0xffffffffu, 0xfffffffeu, 0xfffffffdu, 0x80000000u, 0x7fffffffu, 6u})
            for (unsigned slot_byte = 0; slot_byte < 256; slot_byte += (fog == 1 || fog == 2) ? 1 : 37) {
                const std::uint32_t slot_arg = (slot_byte & 1 ? 0x1234500u : 0u) | slot_byte;
                add(0x100d1d70, "fog " + hex(fog) + " word " + hex(word) + " slot " + hex(slot_arg),
                    setup(subject, record, slot_arg, 1, 1, word), (slot_byte % 7) == 0, stub(data + 0xb600, fog));
            }
}


// The squad scans: a club of 50 slots (some empty), persons with and without
// player records, role bytes, status answers, ids around the global bound,
// ratings around the thresholds and attribute bytes around theirs; the
// status query (0x10187680) is scripted from the person's address.
struct Lcg {
    std::uint32_t state;
    std::uint32_t next() {
        state = state * 1664525u + 1013904223u;
        return state >> 8;
    }
    bool chance(unsigned percent) { return next() % 100 < percent; }
};
bool status_stub(State& s) {
    if (s.pc == 0x10459b50) {  // the attributes owner: answers the record's +0x5cc word (null included)
        Cpu c(s);
        record_call(c);
        c.set(3, c.u32(c.r(3) + 0x5cc));
        s.pc = c.lr();
        return true;
    }
    if (s.pc != 0x10187680) return false;
    Cpu c(s);
    record_call(c);
    const auto person = c.r(4);
    // 0..4 from the address, with junk above the low byte (the original sign-extends the byte).
    const auto status = ((person >> 8) * 7 + (person >> 12)) % 5;
    c.set(3, (status % 3 == 1 ? 0x12300u : 0u) | status);
    s.pc = c.lr();
    return true;
}
void squad_scans() {
    constexpr std::uint32_t club_at = data + 0xd000, persons = data + 0xd400, players = data + 0x11000,
                            status_word = data + 0x130, bound_word = data + 0x134;
    struct Entry { std::uint32_t entry; std::vector<std::uint32_t> roles, attributes; bool prefix = false; };
    const std::vector<Entry> scans{
        {0x100dcc30, {0x14, 0x12, 0x13}, {}},       {0x100dcfc0, {}, {0x44}},       {0x100dd100, {0xf}, {}},
        {0x100dca00, {0x11}, {}, true},           {0x100dcd90, {0x15}, {}, true},
        {0x100dd200, {0x15}, {0x2e}},               {0x100dd380, {0x11}, {0x2e}},
        {0x100dd500, {0x11}, {0x36}},               {0x100dd680, {0x15}, {0x36}},
        {0x100dd7f0, {0x14, 0x12, 0x13}, {0x44, 0x3e}}, {0x100dd9b0, {0x14, 0x12, 0x13}, {0x43, 0x37}}};
    const std::array<std::int16_t, 14> club_ratings{0, 1, 300, 1000, 2500, 4000, 6000, 8000, 10000, 13000, 20000, 32767, -1, -32768};
    for (const auto& scan : scans) {
        for (unsigned variant = 0; variant < 72; ++variant) {
            const unsigned quality = 35 + (variant % 4) * 21;  // the chance each test passes
            const auto rating = club_ratings[variant % club_ratings.size()];
            const bool in_high = variant % 17 == 5, overflow = variant % 5 == 0;
            const bool negative = variant % 6 == 2;  // ids and bound in the signed-negative range
            const std::uint32_t bound = negative ? 0x80000010u + variant % 4 : 18 + variant % 5 * 3;
            add(scan.entry, "club rating " + std::to_string(rating) + " variant " + std::to_string(variant),
                [=](Cpu& c) {
                    put_toc(c, -0x7ba0, status_word);
                    put_toc(c, -0x7be4, bound_word);
                    c.put32(status_word, 0x4321);
                    c.put32(bound_word, bound);
                    Lcg rng{0x1234567u + variant * 7919u + scan.entry};
                    const auto club = in_high ? high + 0x200 : club_at;
                    c.put16(club + 0x1ec, std::uint16_t(rating));
                    for (unsigned i = 0; i < 50; ++i) {
                        const auto person = persons + i * 0x100, player = players + i * 0x100;
                        c.put32(club + 0x98 + i * 4, rng.chance(12) ? 0 : person);
                        c.put32(person, negative ? 0x80000000u + rng.next() % 0x20 : rng.chance(quality + 10) ? 8 + rng.next() % 10 : bound + rng.next() % 5);
                        c.put32(person + 0x14, rng.chance(8) ? 0 : player);
                        for (unsigned b = 0; b < 0x60; ++b) c.put8(player + b, rng.next() % 24 + (rng.chance(10) ? 0xe8 : 0));
                        for (const auto role : scan.roles) if (rng.chance(quality / 2 + 20)) c.put8(player + role, 20);
                        // The rating around the thresholds (0x28..0x91) and sometimes negative.
                        c.put16(player + 4, rng.chance(8) ? 0xff00 + rng.next() % 200 : 20 + rng.next() % (quality + 100));
                        for (const auto offset : scan.attributes) c.put8(player + offset, rng.chance(quality + 8) ? 10 + rng.next() % 12 : rng.next() % 10);
                    }
                    if (scan.prefix) {
                        // The team chain: team word 0 indexes the table; each link can be missing.
                        constexpr std::uint32_t table_slot = data + 0x138, table = data + 0x14000, team = data + 0x14800,
                                                entry_record = data + 0x14900, inner = data + 0x15000, answer = data + 0x16000,
                                                number_pool = data + 0x3000;
                        put_toc(c, -0x7b5c, table_slot);
                        put_toc(c, -0x5614, number_pool);
                        c.put32(table_slot, table);
                        c.put64(number_pool + 0x18, std::bit_cast<std::uint64_t>(4503601774854144.0));
                        c.put64(number_pool + 0x90, std::bit_cast<std::uint64_t>(variant % 2 ? 4.0 : 3.0));
                        c.put64(number_pool + 0x130, std::bit_cast<std::uint64_t>(variant % 3 ? 5.0 : 2.0));
                        const unsigned chain = variant % 9;
                        const std::array<std::pair<unsigned, unsigned>, 7> quotients{{{200, 1}, {3, 2}, {100, 10}, {6, 3}, {3, 1}, {4, 1}, {10, 2}}};
                        const auto [numerator, denominator] = quotients[variant / 9 % 7];
                        c.put32(club + 0x30, chain == 0 ? 0 : team);
                        c.put32(team, 3);
                        c.put32(table + 12, chain == 1 ? 0 : entry_record);
                        c.put32(entry_record + 8, chain == 2 ? 0 : inner);
                        c.put32(inner + 0x5cc, chain == 3 ? 0 : answer);  // 0x10459b50 is scripted: it answers this word
                        c.put8(answer + 4, chain == 4 ? 0 : denominator);
                        c.put8(answer + 0x6c, numerator);
                        c.put8(answer + 0x6d, numerator);
                    }
                    c.set(3, club);
                    c.set(5, 0x7777);
                },
                overflow, status_stub);
        }
        add(scan.entry, "no club", [](Cpu& c) { c.set(3, 0); });
        add(scan.entry, "no club, overflow", [](Cpu& c) { c.set(3, 0); }, true);
    }
}

// 0x100ddb70: the club level; the persons' sort comparator (the TOC word at
// -0x7b56c0's target) is scripted: by the player's rating, best first, ties 0.
bool person_order_stub(State& s) {
    Cpu c(s);
    if (s.pc != c.u32(c.toc(-0x56c0))) return false;
    const auto rating = [&](std::uint32_t item) { return std::int32_t(c.i16(c.u32(c.u32(item) + 0x14) + 4)); };
    record_call(c);
    c.set(3, std::uint32_t(sign(std::int64_t(rating(c.r(4))) - rating(c.r(3)))));
    s.pc = c.lr();
    return true;
}
void club_levels() {
    constexpr std::uint32_t club_a = data + 0xd000, club_b = data + 0xd200, persons = data + 0xd400, players = data + 0x11000,
                            table_slot = data + 0x138, bound_word = data + 0x134, table = data + 0x14000, team = data + 0x14800;
    const std::array<std::int16_t, 9> ratings_a{0, 2000, 4249, 4250, 7000, 7249, 7250, 9000, -300};
    const std::array<int, 9> gaps{-100, 0, 1249, 1250, 1251, 2000, 5000, 3000, 1300};
    const std::array<std::uint8_t, 5> flags{0, 1, 0x80, 0xff, 5};
    for (unsigned variant = 0; variant < 360; ++variant) {
        const auto rating_a = ratings_a[variant % 9];
        const auto rating_b = std::int16_t(rating_a + gaps[variant / 9 % 9]);
        const bool no_b = variant % 13 == 4, same_team = variant % 3 != 0, no_a = variant == 359;
        const unsigned players_count = variant % 11 == 0 ? 0 : variant % 7 == 1 ? 1 : variant % 5 == 2 ? 3 : variant % 4 == 1 ? 8 : 18;
        const unsigned strength = 20 + variant * 37 % 150;
        const auto flag_a = flags[variant / 3 % 5], flag_b = flags[variant / 5 % 5];
        add(0x100ddb70, "a rating " + std::to_string(rating_a) + " b " + (no_b ? std::string("none") : std::to_string(rating_b)) +
                            " persons " + std::to_string(players_count) + " strength " + std::to_string(strength) + " variant " + std::to_string(variant),
            [=](Cpu& c) {
                put_toc(c, -0x7984, table_slot);
                put_toc(c, -0x7be4, bound_word);
                c.put32(table_slot, table);
                const bool negative = variant % 8 == 3;
                c.put32(bound_word, negative ? 0x80000010u : 30);
                Lcg rng{0xabcdefu + variant * 104729u};
                c.put16(club_a + 0x1ec, std::uint16_t(rating_a));
                c.put16(club_b + 0x1ec, std::uint16_t(rating_b));
                c.put32(club_a + 0x30, team);
                c.put32(club_b + 0x30, same_team ? team : team + 0x100);
                c.put32(club_a, 3);
                c.put32(club_b, 5);
                c.put8(table + 3 * 8 + 4, flag_a);
                c.put8(table + 5 * 8 + 4, flag_b);
                for (unsigned i = 0; i < 50; ++i) {
                    const auto person = persons + i * 0x100, player = players + i * 0x100;
                    c.put32(club_a + 0x98 + i * 4, i < players_count ? person : 0);
                    c.put32(person, negative ? 0x80000000u + rng.next() % 0x20 : rng.chance(85) ? 10 + rng.next() % 15 : 30 + rng.next() % 5);
                    c.put32(person + 0x14, rng.chance(5) ? 0 : player);
                    c.put16(player + 4, rng.chance(5) ? 0xff80 : strength + rng.next() % 40);
                }
                c.set(3, no_a ? 0 : club_a);
                c.set(4, no_b ? 0 : club_b);
            },
            variant % 6 == 0, [](State& s) { return person_order_stub(s); });
    }
}

// 0x100d3f30: a table of 20 clubs, some with an owner and a code word the real club_of_code
// (0x100ce320) maps to a target club (every TOC cell of its tables holds the same value, so the
// word is a member and maps to that index; -1 maps to no club). The real callees run on both
// sides: club_of_code, 0x100c8930 (removal) and 0x100c82a0 (add), with the list slots filled with
// persons of ids around the global limit and kind bytes that pick the removal's lists. Only calls
// outside the module are scripted: the assert macro's four, 0x1014c2e0 and the sort comparator
// (not recorded: a 50-person sort calls it hundreds of times; the sorted order is compared).
void club_relink() {
    namespace code = cm::aot::database::records::club_code;
    constexpr std::uint32_t cells = data + 0x10000, table = data + 0x4000, persons = data + 0x12000,
                            table_cell = data + 0x13c, order_cell = data + 0x140, kind_cell = data + 0x1c0,
                            count_cell = data + 0x148, low_cell = data + 0x14c, limit_cell = data + 0x150,
                            owner = data + 0x1200;
    std::map<std::int32_t, std::uint32_t> cell_of;
    const auto note = [&](std::int32_t at) { cell_of.try_emplace(at, std::uint32_t(cell_of.size())); };
    for (const auto& pair : code::pairs) {
        note(pair.code);
        note(pair.index);
    }
    constexpr std::int32_t owner_types[3] = {-0x7a18, -0x7af4, -0x79fc};
    struct Variant { std::uint32_t target; std::vector<unsigned> active; unsigned seed; };
    std::vector<Variant> variants;
    for (const std::uint32_t target : {0xffffffffu, 0u, 3u, 7u, 19u})
        for (const auto& active : std::vector<std::vector<unsigned>>{{1}, {1, 2, 3}, {0, 5, 6, 12}, {19, 4}})
            for (const unsigned seed : {1u, 2u, 3u}) variants.push_back({target, active, seed});
    for (const auto& variant : variants) {
        std::string text = "target " + hex(variant.target) + " active " + std::to_string(variant.active.size()) + " seed " + std::to_string(variant.seed);
        add(0x100d3f30, text, [=](Cpu& c) {
            for (const auto& [at, index] : cell_of) {
                put_toc(c, at, cells + index * 4);
                c.put32(cells + index * 4, variant.target);
            }
            for (unsigned k = 0; k < 3; ++k) {
                put_toc(c, owner_types[k], data + 0x1600 + 4 * k);
                c.put32(data + 0x1600 + 4 * k, 7 + k);
            }
            c.put32(owner + 0x68, 7);
            put_toc(c, -0x7be8, table_cell);
            put_toc(c, -0x7980, order_cell);
            put_toc(c, -0x7b80, kind_cell);
            put_toc(c, -0x7b50, count_cell);
            put_toc(c, -0x7b04, low_cell);
            put_toc(c, -0x7be4, limit_cell);
            put_toc(c, -0x7ba8, data + 0x154);
            put_toc(c, -0x7bac, data + 0x158);
            put_toc(c, -0x5618, pool);
            c.put32(table_cell, table);
            c.put32(order_cell, 0x100cd390);  // the comparator's entry (the stub answers); its TOC follows
            c.put32(order_cell + 4, toc);
            c.put32(count_cell, 20);
            c.put32(low_cell, 3);
            c.put32(limit_cell, variant.seed == 3 ? 0x80000020u : 40);
            c.put16(kind_cell, 0x7e1);
            c.put16(kind_cell + 2, 0x123);
            c.put8(kind_cell + 4, 7);
            Lcg rng{0x51ed270bu + variant.seed * 7919u};
            for (unsigned k = 0; k < 80; ++k) {
                const auto person = persons + k * 0x80;
                c.put32(person, variant.seed == 3 ? 0x80000000u + rng.next() % 0x40 : rng.chance(70) ? rng.next() % 24 : 24 + rng.next() % 22);
                c.put32(person + 0x14, 0);
                const std::array<std::uint8_t, 7> kinds{0, 1, 2, 8, 9, 10, 11};
                c.put8(person + 0x60, kinds[rng.next() % kinds.size()]);
                c.put8(person + 0x62, kinds[rng.next() % kinds.size()]);
            }
            const auto fill = [&](std::uint32_t club, std::uint32_t offset, unsigned count) {
                for (unsigned i = 0; i < count; ++i)
                    c.put32(club + offset + i * 4, rng.chance(60) ? persons + (rng.next() % 80) * 0x80 : 0);
            };
            for (unsigned i = 0; i < 20; ++i) {
                const auto club = table + i * 0x248;
                const bool active = std::ranges::find(variant.active, i) != variant.active.end();
                c.put32(club, active ? variant.target : 5);
                c.put32(club + 0x2c, active ? owner : 0);
                c.put8(club + 0x244, 1);
                fill(club, 0x98, 50);
                fill(club, 0x160, 5);
                fill(club, 0x174, 7);
                fill(club, 0x190, 3);
                fill(club, 0x90, 1);
                fill(club, 0x94, 1);
            }
        }, variant.seed == 2, [](State& s) {
            const bool assertion = std::ranges::find(assert_calls, s.pc) != assert_calls.end();
            Cpu c(s);
            if (s.pc == 0x100cd390) {
                const auto id = [&](std::uint32_t item) {
                    const auto person = c.u32(item);
                    return person ? std::int32_t(c.u32(person)) : -1;
                };
                const auto answer = id(c.r(3)) < id(c.r(4)) ? -1 : id(c.r(3)) > id(c.r(4)) ? 1 : 0;
                c.set(3, std::uint32_t(answer));
                s.pc = c.lr();
                return true;
            }
            if (!assertion && s.pc != 0x1014c2e0) return false;
            record_call(c);
            c.set(3, assertion ? (s.pc == 0x10768810 ? data + 0xb000 : data + 0xb100) : (c.r(3) >> 4) & 1);
            s.pc = c.lr();
            return true;
        });
    }
}

// 0x100c8930: every kind (including past 15 and negative bytes) against persons present at the first or
// last slot, absent or elsewhere, in the range cases (club word below the limit, not below with the
// range position 0 and 1 for the kind that walks to the owner's club), with and without the date, and
// the two flags. The two records-block callees run for real (club_at_bias, range_position).
void removal_cases() {
    constexpr std::uint32_t club = data + 0x4000, person = data + 0x5000, other = data + 0x5100, date = data + 0x5400,
                            table = data + 0x6000, owner = data + 0x1200, low_cell = data + 0x14c, high_cell = data + 0x148,
                            table_cell = data + 0x13c, ready_cell = data + 0x150;
    const std::array<std::uint32_t, 20> kinds{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0x80, 0xff, 0x1b};
    for (const std::uint32_t kind : kinds)
        for (unsigned placement = 0; placement < 5; ++placement)
            for (const std::uint32_t club_word : {5u, 15u, 30u})
                for (unsigned options = 0; options < 12; ++options) {
                    const bool with_date = options & 1, flag_a = options & 2;
                    const std::uint32_t flag_b_word = options / 4 == 1 ? 0x201 : options / 4 == 2 ? 0x200 : 0;  // 0x200: nonzero, low byte zero
                    if (placement > 2 && club_word != 5 && kind != 5) continue;
                    add(0x100c8930, "kind " + hex(kind) + " placement " + std::to_string(placement) + " club word " +
                                        std::to_string(club_word) + " options " + std::to_string(options),
                        [=](Cpu& c) {
                            put_toc(c, -0x7b04, low_cell);
                            put_toc(c, -0x7b50, high_cell);
                            put_toc(c, -0x7be8, table_cell);
                            put_toc(c, -0x7b98, ready_cell);
                            put_toc(c, -0x7ba8, data + 0x154);
                            put_toc(c, -0x7bac, data + 0x158);
                            put_toc(c, -0x5618, pool);
                            c.put32(low_cell, 3);
                            c.put32(high_cell, 20);
                            c.put32(table_cell, table);
                            c.put32(ready_cell, 1);
                            c.put32(club, club_word);
                            c.put32(club + 0x2c, owner);
                            c.put32(owner + 0x68, 2);
                            c.put8(person + 0x60, kind);
                            c.put8(person + 0x62, kind ^ (placement == 4 ? 1 : 0));
                            c.put32(person + 0x20, 0x77777777);
                            c.put32(person + 0x24, 0x88888888);
                            for (unsigned i = 0; i < 20; ++i) {
                                c.put32(table + i * 0x248 + 0x90, i == 19 ? (placement == 1 ? 0 : person) : other);
                                c.put32(table + i * 0x248, 7);
                            }
                            // Fill every slot with another person, then place `person` by the case.
                            const auto fill = [&](std::uint32_t offset, unsigned count) {
                                for (unsigned i = 0; i < count; ++i) c.put32(club + offset + i * 4, other);
                                if (placement == 0) c.put32(club + offset, person);
                                else if (placement == 1) c.put32(club + offset + (count - 1) * 4, person);
                                else if (placement == 3) c.put32(club + offset + (count / 2) * 4, person);
                            };
                            fill(0x84, 3);
                            fill(0x98, 50);
                            fill(0x160, 5);
                            fill(0x174, 7);
                            fill(0x190, 3);
                            for (const std::uint32_t word : {0x80u, 0x90u, 0x94u})
                                c.put32(club + word, placement == 2 || placement == 4 ? other : person);
                            c.put16(date, 0x7e1);
                            c.put16(date + 2, 0x8123);
                            c.put8(date + 4, 9);
                            c.set(3, club);
                            c.set(4, person);
                            c.set(5, with_date ? date : 0);
                            c.set(6, flag_a ? 0x1ff : 0x100);
                            c.set(7, flag_b_word);
                        },
                        options == 5, [](State& s) {
                            if (std::ranges::find(assert_calls, s.pc) == assert_calls.end()) return false;
                            Cpu c(s);
                            record_call(c);
                            c.set(3, s.pc == 0x10768810 ? data + 0xb000 : data + 0xb100);
                            s.pc = c.lr();
                            return true;
                        });
                }
    // No club, and no person (the assert).
    add(0x100c8930, "no club", [](Cpu& c) { c.set(3, 0); c.set(4, person); });
    add(0x100c8930, "no person", [](Cpu& c) {
        put_toc(c, -0x7ba8, data + 0x154);
        put_toc(c, -0x7bac, data + 0x158);
        put_toc(c, -0x5618, pool);
        c.set(3, club);
        c.set(4, 0);
    }, false, [](State& s) {
        if (std::ranges::find(assert_calls, s.pc) == assert_calls.end()) return false;
        Cpu c(s);
        record_call(c);
        c.set(3, s.pc == 0x10768810 ? data + 0xb000 : data + 0xb100);
        s.pc = c.lr();
        return true;
    });
}

// 0x100d71a0 and 0x100d8260: the trait masks. The noise table, its pointer, bound and seed are set per
// variant so that the pointer wraps (one draw of the generator) at the k-th draw, at every draw, never,
// exactly at the bound, or through an address above 0x7fffffff (the compare is unsigned); the record's
// attribute bytes and the scripted statistic answers come in modes that pass the tests, fail them or mix.
// The attribute routine 0x100d2330 runs for real (with the pool of constants it reads); the statistics
// owner, the transfer manager, the tactics callees and the id rating are scripted and record their
// arguments, the registers a callee saves, f0-f5, f30, f31 and the FPSCR. The callees that production pages call in
// place as C++ owners (0x103f43b0, 0x103b05c0, 0x103af570, 0x10111240) cannot be scripted on the original's
// side, so they run for real on both: the transfer manager on an object whose records give it both answers, the tactics callees on a club of zeros (no row hits) with a row table that holds the
// person's id in one of 20 slots or in none, and a record the last of them reads a halfword from.
void record_floats(Cpu& c) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    for (const unsigned n : {0u, 1u, 2u, 3u, 4u, 5u, 30u, 31u}) {
        c.put32(at, std::uint32_t(c.s.r<8>(guest::reg::fpr(n)) >> 32));
        c.put32(at + 4, std::uint32_t(c.s.r<8>(guest::reg::fpr(n))));
        at += 8;
    }
    for (unsigned b = 1280; b < 1312; b += 4) {  // the FPSCR, one byte per bit
        c.put32(at, std::uint32_t(c.s.r<4>(b)));
        at += 4;
    }
    c.put32(trace_at, (at - trace_at - 4) / 4);
}
struct TraitCase {
    unsigned seed, wrap, mode;
};
std::uint32_t mix(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    std::uint32_t h = a * 0x9e3779b1u ^ (b + 0x7f4a7c15u) * 0x85ebca6bu ^ (c + 0x165667b1u) * 0xc2b2ae35u;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    return h;
}
// A statistic answer: random from a menu around the pool constants, or from its high or low half.
double stat_answer(const TraitCase& v, std::uint32_t number, std::uint32_t mode) {
    static constexpr std::array<double, 24> menu{-5.0, 0.0, 0.5, 1.0, 2.0, 2.5, 4.0, 5.0, 6.0, 8.0, 9.0, 11.0, 12.0, 13.0,
                                                  14.0, 40.0, 100.0, 140.0, 300.0, 1e9, 70000.0, -0.0, 200.0, 1000.0};
    const auto h = mix(v.seed, number, mode);
    switch (v.mode) {
    case 1: case 4: return menu[12 + h % 12];  // high
    case 2: case 3: return menu[h % 8];        // low
    default:
        if (h % 41 == 0) return std::numeric_limits<double>::quiet_NaN();
        return menu[h % menu.size()];
    }
}
bool trait_stub(State& s, const TraitCase& v) {
    Cpu c(s);
    switch (s.pc) {
    case 0x10303560:
        record_call(c);
        record_floats(c);
        c.set_f(1, stat_answer(v, c.r(6), c.r(5)));
        break;
    case 0x10739890: case 0x10768810: case 0x1077b720: case 0x10769b60:  // the assert macro's four (a null record fails 0x100d2330)
        record_call(c);
        record_floats(c);
        c.set(3, s.pc == 0x10768810 ? data + 0xb000 : data + 0xb100);
        break;
    case 0x1077fc28:  // the block move behind 0x10739ef0 (the manager's list copies an id): source r3, destination r4, size r5
        for (std::uint32_t i = 0; i < c.r(5); ++i) c.put8(c.r(4) + i, c.u8(c.r(3) + i));
        c.put32(c.sp() + 0x14, c.r(2));  // the import glue saves the TOC where the caller reloads it
        break;
    case 0x1077fc58:  // the system call that gives an emptied pool back (DisposePtr): nothing to do
        c.put32(c.sp() + 0x14, c.r(2));
        c.set(3, 0);
        break;
    case 0x10187650: {  // the id's rating: a halfword (with junk above it)
        record_call(c);
        record_floats(c);
        const auto value = std::int32_t(mix(v.seed, c.r(4), 7) % 2200) - 300;
        c.set(3, (std::uint32_t(value) & 0xffff) | (v.seed & 4 ? 0x12340000u : 0));
        break;
    }
    case 0x10216830:
        record_call(c);
        record_floats(c);
        c.set(3, std::uint32_t(std::int32_t(mix(v.seed, c.r(4), 8) % 500) - 100));
        break;
    default:
        return false;
    }
    s.pc = c.lr();
    return true;
}

void trait_masks() {
    constexpr std::uint32_t person_at = data + 0x1000, record_at = data + 0x1100, transfer_cell = data + 0x200,
                            flag_cell = data + 0x204, object_cell = data + 0x208, bound_cell = data + 0x210,
                            pointer_cell = data + 0x214, seed_cell = data + 0x218, table_cell = data + 0x21c,
                            team_cell = data + 0x220, id_cell = data + 0x224, floats = data + 0x300,
                            doubles = data + 0x400, noise_table = data + 0x4000, id_table = data + 0x5000,
                            high_table = high + 0x100, transfer_object = data + 0x3000, flag_table = data + 0x3a00,
                            team_object = data + 0x2100, club_record = data + 0x2800, rows = data + 0x8800,
                            records_a = data + 0xa000, records_b = data + 0xe000;
    for (const std::uint32_t entry : {0x100d71a0u, 0x100d8260u})
        for (const unsigned mode : {0u, 1u, 2u, 3u, 4u})
            for (const unsigned wrap : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u,
                                        20u, 21u, 22u, 23u, 24u, 25u, 26u, 98u, 99u, 100u})
                for (unsigned seed = 0; seed < (wrap == 0 || wrap >= 98 ? 6u : 2u); ++seed) {
                    const TraitCase v{seed * 7919u + mode * 31u + wrap * 101u + (entry & 0xff), wrap, mode};
                    add(entry, "trait mask mode " + std::to_string(mode) + " wrap " + std::to_string(wrap) + " seed " + std::to_string(seed),
                        [=](Cpu& c) {
                            put_toc(c, -0x7b40, transfer_cell);
                            put_toc(c, -0x7bac, flag_cell);
                            put_toc(c, -0x7b3c, object_cell);
                            put_toc(c, -0x56bc, floats);
                            put_toc(c, -0x5614, doubles);
                            put_toc(c, -0x7b88, bound_cell);
                            put_toc(c, -0x7b84, pointer_cell);
                            put_toc(c, -0x7b90, seed_cell);
                            put_toc(c, -0x7b9c, table_cell);
                            put_toc(c, -0x7a6c, team_cell);
                            put_toc(c, -0x7ba0, id_cell);
                            put_toc(c, -0x7ba8, data + 0x154);
                            put_toc(c, -0x5618, pool);
                            put_toc(c, -0x7b50, data + 0x230);  // the club count and the low bound 0x100ce250 compares an id with
                            put_toc(c, -0x7b04, data + 0x234);
                            c.put32(data + 0x230, 20);
                            c.put32(data + 0x234, 3);
                            Lcg rng{0x7a11u + v.seed * 104729u};
                            const bool high_noise = wrap == 100;
                            const auto table = high_noise ? high_table : noise_table;
                            put_toc(c, -0x7b8c, table);
                            for (unsigned i = 0; i < 64; ++i) c.put16(table + i * 2, rng.chance(10) ? 0xffffu : rng.next() % 0x8000);
                            const unsigned start = rng.next() % 12;
                            c.put32(pointer_cell, table + start * 2);
                            // The draw k wraps when pointer + 2k > bound: bound = pointer + 2k - 1; every third k puts
                            // the bound at pointer + 2k (the draw k does not wrap, the next one does).
                            std::uint32_t bound = 0xfffffff0u;
                            if (wrap >= 1 && wrap <= 40) bound = table + start * 2 + 2 * wrap - (wrap % 3 == 2 ? 0 : 1);
                            if (wrap == 99) bound = 0;
                            if (high_noise) bound = data + 0x2000;
                            c.put32(bound_cell, bound);
                            c.put32(seed_cell, seed % 3 == 2 ? 0x80000010u + rng.next() % 64 : rng.next() % 0x8000);
                            c.put32(c.r(2) - 0x34c4, rng.next() * 7919u + 1);
                            c.put32(object_cell, 0x12345678);
                            c.put32(transfer_cell, v.seed % 5 == 3 ? 0u : transfer_object);
                            c.put32(table_cell, id_table);
                            c.put32(team_cell, team_object);
                            c.put32(id_cell, v.seed % 7 == 5 ? 0u : data + 0x2200);
                            c.put8(flag_cell, 1);
                            for (unsigned i = 0; i < 0x3000; ++i) c.put8(id_table + i, rng.next());
                            // The constants the routines (and the rating formula) read.
                            for (const auto& [offset, value] : std::array<std::pair<std::uint32_t, double>, 17>{{
                                     {0x18, 4503601774854144.0}, {0x40, 5.0}, {0x48, 0.5}, {0x60, 0.1}, {0x68, 2.0}, {0xa0, 2.0},
                                     {0xb0, 10.0}, {0xb8, 20.0}, {0xc0, 3.0}, {0xc8, 1.0}, {0xf0, 1.0}, {0xf8, 12.0},
                                     {0x100, 1.5}, {0x108, 2.5}, {0x110, 8.0}, {0x118, 1.0}, {0x130, 5.0}}})
                                c.put64(doubles + offset, std::bit_cast<std::uint64_t>(value));
                            for (const auto& [offset, value] : std::array<std::pair<std::uint32_t, float>, 6>{{
                                     {0, 1.0f}, {4, 6.0f}, {8, 100.0f}, {0xc, 3.0f}, {0x10, 1.5f}, {0x14, 2.0f}}})
                                c.put_f32(floats + offset, value);
                            // The person and the player record. Modes: 0 random bytes, 1 and 2 high (0x7f), 3 and 4 low.
                            const auto id = std::uint32_t(rng.chance(15) ? 90 + rng.next() % 30 : rng.next() % 90);
                            c.put32(person_at, id);
                            // Some persons have no player record: the routines then read the bytes of the low page.
                            const bool no_record = v.seed % 13 == 6;
                            c.put32(person_at + 0x14, no_record ? 0u : record_at);
                            if (no_record) {
                                c.s.memory.map(0, 0x1000);
                                for (unsigned i = 0; i < 0x100; ++i) c.put8(i, rng.next());
                            }
                            const bool high_bytes = mode == 1 || mode == 2, low_bytes = mode == 3 || mode == 4;
                            for (unsigned i = 0; i < 0x80; ++i) {
                                std::uint8_t byte = rng.next();
                                if (i >= 0x1b && i <= 0x44) {
                                    if (high_bytes) byte = rng.chance(90) ? 0x7f : byte;
                                    if (low_bytes) byte = rng.chance(90) ? 0x80 : byte;
                                }
                                if (i >= 0xf && i <= 0x19) byte = rng.chance(45) ? 20 : (low_bytes ? 3 : byte);
                                c.put8(record_at + i, byte);
                            }
                            for (unsigned i = 0; i < 0x80; ++i) c.put8(person_at + 0x40 + i, high_bytes ? 0x7f : low_bytes ? 0x80 : rng.next());
                            c.put16(record_at + 4, rng.chance(20) ? 0xff00u + rng.next() % 256 : rng.next() % 2000);
                            c.put16(record_at + 0xa, rng.chance(20) ? 0x8000u + rng.next() % 256 : rng.next() % 3000);
                            c.set(3, person_at);
                            c.set(4, entry == 0x100d71a0 ? (v.seed % 11 == 4 ? 0u : 1u) : (v.seed % 11 == 4 ? 0u : club_record));
                            // The transfer manager: the flag byte (bit 0x10) of the person is clear.
                            c.put32(transfer_object + 0x9a0, flag_table);
                            for (unsigned i = 0; i < 1300; ++i) c.put8(flag_table + i, rng.next() & ~0x10u);
                            // Bit 0x10 set: 0x103eaea0 looks the person's id up in the manager's array of three records
                            // (+0xc), and when the one found has no list (+0) and passes 0x103fb390 (byte +0x34 of 0x13) the
                            // person's id word +0x28 (the record's own index) names a record whose +0x34 is 0x13: answer 1.
                            // Other bytes (0x14, 2, 0xf with +0x36 of 3 or more) make 0x103fb390 answer 1 (0x103eaea0: 0), and
                            // a missing record or an id at or above the bound (TOC -0x7be4) make it answer 0 as well.
                            // The list the manager builds for the answer allocates: a working allocator (context, size classes
                            // and one raw arena), as the allocator checks set it up.
                            {
                                constexpr std::uint32_t context = data + 0x14000, size_classes = data + 0x14100, arena = data + 0x14800,
                                                        size = 0x3000;
                                for (unsigned i = 0; i < 52; i += 4) c.put32(context + i, 0);
                                put_toc(c, -0x3b74, context);
                                put_toc(c, -0x3b70, size_classes);
                                c.put8(c.r(2) - 0x3510, 1);
                                for (unsigned i = 0; i < 6; ++i) c.put32(size_classes + i * 4, std::array<std::uint32_t, 6>{4, 12, 28, 44, 60, 76}[i]);
                                for (const auto& [at, value] : std::array<std::pair<std::uint32_t, std::uint32_t>, 11>{{
                                         {0, arena}, {4, arena}, {16, size - 32}, {20, size | 3}, {24, size - 32}, {28, arena | 1},
                                         {32, arena + 24}, {36, arena + 24}, {size - 12, size - 32}, {size - 8, size | 3},
                                         {size - 4, arena + 24}}})
                                    c.put32(arena + at, value);
                                c.put32(context, arena);
                                c.s.allocated[arena] = size;
                            }
                            put_toc(c, -0x7be4, data + 0x238);
                            c.put32(data + 0x238, rng.chance(12) ? 5u : 200u);
                            if (rng.chance(75)) c.put8(flag_table + id * 10, c.u8(flag_table + id * 10) | 0x10);
                            c.put8(flag_table + id * 10 + 2, 0);  // 0x103eb020's second lookup stops here (it would need the contract manager)
                            c.put32(transfer_object, data + 0x12000);
                            c.put32(transfer_object + 4, 3);
                            const unsigned found = rng.next() % 4;
                            static constexpr std::array<std::uint8_t, 6> kinds{0x13, 0x13, 0x13, 0x14, 0x03, 0x0f};
                            for (unsigned i = 0; i < 3; ++i) {
                                const auto entry_at = data + 0x12100 + i * 0x100;
                                c.put32(data + 0x12000 + i * 4, entry_at);
                                for (unsigned b = 0; b < 0x100; ++b) c.put8(entry_at + b, 0);
                                c.put32(entry_at + 0xc, i == found ? id : id + 1 + i);
                                c.put32(entry_at + 0x28, i);
                                c.put8(entry_at + 0x34, kinds[rng.next() % kinds.size()]);
                                c.put8(entry_at + 0x36, rng.next() % 6);
                            }
                            // The tactics objects: the club's row index k, the row's halfword h (a record index) and
                            // the 20 entries compared with the person's id.
                            c.put32(team_object + 0x5c4, rows);
                            c.put32(team_object + 0x5c0, 2);
                            c.put32(team_object + 0x5bc, records_a);
                            c.put32(team_object + 0x5b4, records_b);
                            for (unsigned i = 0; i < 0x100; ++i) c.put8(club_record + i, 0);
                            const auto k = rng.next() % 4, h = rng.next() % 4, hit = rng.next() % 26;
                            c.put32(club_record, k);
                            for (unsigned i = 0; i < 4 * 0x94; ++i) c.put8(rows + i, rng.next());
                            c.put16(rows + k * 0x94, h);
                            for (unsigned j = 0; j < 20; ++j) c.put32(rows + k * 0x94 + 4 + j * 4, j == hit ? id : id + 1 + rng.next() % 3);
                            for (unsigned i = 0; i < 0x4000; i += 2) {
                                c.put16(records_a + i, rng.next());
                                c.put16(records_b + i, rng.next());
                            }
                        },
                        (seed + wrap) % 3 == 0, [v](State& s) { return trait_stub(s, v); });
                }
    // Null person and the zero argument.
    for (const std::uint32_t entry : {0x100d71a0u, 0x100d8260u}) {
        const auto base = [=](Cpu& c) {
            put_toc(c, -0x7b3c, object_cell);
            put_toc(c, -0x56bc, floats);
            put_toc(c, -0x5614, doubles);
            put_toc(c, -0x7b88, bound_cell);
            put_toc(c, -0x7b84, pointer_cell);
            put_toc(c, -0x7b90, seed_cell);
            put_toc(c, -0x7b8c, noise_table);
            c.put32(pointer_cell, noise_table);
            c.put32(bound_cell, 0xfffffff0u);
        };
        add(entry, "null person", [=](Cpu& c) { base(c); c.set(3, 0); c.set(4, 1); });
        add(entry, "zero argument", [=](Cpu& c) { base(c); c.set(3, person_at); c.set(4, 0); }, true);
    }
}
}

std::vector<Fixture> search_fixtures() {
    club_id_sorter();
    byte_sorter();
    ranked_compactor();
    search_dispatcher();
    slot_visibility();
    squad_scans();
    club_levels();
    club_relink();
    removal_cases();
    trait_masks();
    return std::move(fixtures);
}
}
