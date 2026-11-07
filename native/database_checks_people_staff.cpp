// Fixtures of the people block, part "staff" (database_checks_people.cpp): 0x100dc4d0 (best staff candidate)
// and 0x100dbc20 (no better candidate). The world is built from a seeded mixer: a person and a club with up
// to 50 slotted candidates, each failing one gate of the score or passing them all, with the person's windows
// (id % 6 bytes, id % 28 days) on both sides. The callees are scripted from the seed and record their
// registers: the calendar difference 0x10718130, the contract lookup 0x10082f90, the transfer manager
// 0x103f43b0 and the statistics 0x10303560 (f1, f31, f0 recorded); the position mask runs for real.
#include "database_checks_people_detail.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <vector>

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;

constexpr std::uint32_t person_at = data + 0x1000, record_at = data + 0x1100, prefs_at = data + 0x1200,
                        club_at = data + 0x2000, candidates_at = data + 0x3000, contracts_at = data + 0xb400,
                        stats_object = data + 0x10800, floats_at = data + 0x10900, doubles_at = data + 0x10a00,
                        transfers_cell = data + 0x10c00, stats_cell = data + 0x10c04, contracts_cell = data + 0x10c08,
                        count_cell = data + 0x10c0c, calendar_word = data + 0x10c10, fresh_at = data + 0x11000,
                        manager = data + 0x1d800, flag_table = data + 0xe000;
constexpr unsigned stride = 0x200;

std::uint32_t hash(std::uint32_t a, std::uint32_t b, std::uint32_t d) {
    std::uint32_t h = a * 0x9e3779b1u ^ (b + 0x7f4a7c15u) * 0x85ebca6bu ^ (d + 0x165667b1u) * 0xc2b2ae35u;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    return h;
}

struct Params {
    std::uint32_t seed;
    unsigned pass_percent;   // chance that a candidate passes each gate
    unsigned slots;          // non-empty slots
    unsigned contract_percent, transfer_percent;
    unsigned head;           // 0 passes; 1 byte 0x65 fails; 2 byte 0x63 fails; 3 days fail; 4 null stats cell
    bool null_person = false, null_club = false, with_high = false;
    unsigned x_kind = 0;     // second routine: 0 a slot's candidate, 1 fresh, 2 the person, 3 null
    bool overflow = false;
    bool ties = false;       // odd slots hold a copy of the slot before; the stubs answer alike, so the scores tie
};

std::uint32_t contract_of(std::uint32_t candidate) { return contracts_at + ((candidate & 0xffff) / stride) * 0x80; }

// The statistics answers: around the thresholds the routine builds (2.5 + 1.5 = 4 and 0.5 + 2.5 = 3 from the
// object's 10.0 / 4.0), the pool's single 3.0, zeros, NaN and big values.
double stat_answer(std::uint32_t h) {
    static constexpr std::array<double, 18> menu{0.0, 1.0, 2.0, 2.5, 3.0, 3.5, 3.9, 4.0, 4.1, 5.0, 6.0,
                                                  100.0, -1.0, 1e9, -0.0, 3.4, 2.9, 3.1};
    if (h % 31 == 0) return std::numeric_limits<double>::quiet_NaN();
    return menu[h % menu.size()];
}

// A lean trace record (the 50 candidates' calls must fit the 0x18000-0x1d000 trace area): the target, LR, the
// first `arguments` registers, r17-r31 (the registers the routines keep) and cr0.
void record_lean(Cpu& c, unsigned arguments) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) {
        c.put32(at, value);
        at += 4;
    };
    add(c.s.pc);
    add(c.lr());
    for (unsigned r = 3; r < 3 + arguments; ++r) add(c.r(r));
    for (unsigned r = 17; r < 32; ++r) add(c.r(r));
    add(c.cr(0));
    c.put32(trace_at, (at - trace_at - 4) / 4);
}

void record_floats(Cpu& c) {
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    for (const unsigned n : {0u, 1u, 31u}) {
        c.put32(at, std::uint32_t(c.s.r<8>(guest::reg::fpr(n)) >> 32));
        c.put32(at + 4, std::uint32_t(c.s.r<8>(guest::reg::fpr(n))));
        at += 8;
    }
    c.put32(trace_at, (at - trace_at - 4) / 4);
}

bool staff_stub(State& s, const Params& p) {
    Cpu c(s);
    const auto next = [&](unsigned k) {
        const auto counter = answer_counters + 4 * k;
        const auto index = c.u32(counter);
        c.put32(counter, index + 1);
        return index;
    };
    switch (s.pc) {
    case 0x10718130: {
        const auto index = next(0);
        record_lean(c, 3);
        static constexpr std::array<std::uint32_t, 10> tails{0, 0x15d, 0x15e, 0x15f, 0x7fffffff, 0x80000000, 0xfffffffb, 1, 400, 0x100};
        if (index == 0) {  // the head's difference against id % 28 + 14
            const auto id = std::int32_t(c.u32(person_at));
            const auto limit = id - 28 * (id / 28) + 14;
            c.set(3, std::uint32_t(p.head == 3 ? limit - 1 - std::int32_t(hash(p.seed, 1, 0) % 3) : limit + std::int32_t(hash(p.seed, 2, 0) % 3)));
        } else {
            c.set(3, tails[hash(p.seed, p.ties ? 1 : index, 3) % tails.size()]);
        }
        break;
    }
    case 0x10082f90: {
        const auto index = next(1);
        record_lean(c, 3);
        c.set(3, hash(p.seed, p.ties ? 0 : index, 4) % 100 < p.contract_percent ? contract_of(c.r(4)) : 0);
        break;
    }
    // The system calls behind the transfer manager's candidate list (as the trait-mask checks stub them): the
    // block move that copies an id (source r3, destination r4, size r5) and the pointer disposal.
    case 0x1077fc28:
        for (std::uint32_t i = 0; i < c.r(5); ++i) c.put8(c.r(4) + i, c.u8(c.r(3) + i));
        c.put32(c.sp() + 0x14, c.r(2));
        break;
    case 0x1077fc58:
        c.put32(c.sp() + 0x14, c.r(2));
        c.set(3, 0);
        break;
    case 0x10303560: {
        const auto index = next(3);
        record_lean(c, 4);
        record_floats(c);
        c.set_f(1, stat_answer(hash(p.seed, p.ties ? 0 : index, c.r(6))));
        break;
    }
    default:
        return false;
    }
    s.pc = c.lr();
    return true;
}

// The transfer manager (0x103f43b0 runs for real: it has a C++ owner, which no stub reaches). A person is a
// "special staff offer" (answer 1) when the bit 0x10 of his status byte is set and one of the manager's three
// records names him with kind 0x13; a person without the bit gets 0. The same world as the trait-mask checks
// (database_checks_search.cpp), with the allocator the answer-1 path needs for its candidate list.
void transfer_world(Cpu& c, const Params& p, Mix& mix, const std::vector<std::uint32_t>& ids) {
    const auto toc = [&](std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); };
    c.put32(manager + 0x9a0, flag_table);
    for (unsigned i = 0; i < 1000 * 10 + 16; ++i) c.put8(flag_table + i, mix.next() & ~0x10u);
    std::vector<std::uint32_t> marked;
    for (const auto id : ids) {
        if (id >= 200) continue;
        // (a few marked people only: each answer-1 call leaks a list from the small arena)
        if (marked.size() < 1 && mix.below(100) >= p.transfer_percent) {
            c.put8(flag_table + id * 10, c.u8(flag_table + id * 10) | 0x10);
            marked.push_back(id);
        }
        c.put8(flag_table + id * 10 + 2, 0);  // 0x103eb020's second lookup stops here (it would need the contract manager)
    }
    constexpr std::uint32_t context = data + 0x14000, size_classes = data + 0x14100, arena = data + 0x14800, size = 0x3000;
    for (unsigned i = 0; i < 52; i += 4) c.put32(context + i, 0);
    toc(-0x3b74, context);
    toc(-0x3b70, size_classes);
    c.put8(c.r(2) - 0x3510, 1);
    for (unsigned i = 0; i < 6; ++i) c.put32(size_classes + i * 4, std::array<std::uint32_t, 6>{4, 12, 28, 44, 60, 76}[i]);
    for (const auto& [at, value] : std::array<std::pair<std::uint32_t, std::uint32_t>, 11>{{
             {0, arena}, {4, arena}, {16, size - 32}, {20, size | 3}, {24, size - 32}, {28, arena | 1},
             {32, arena + 24}, {36, arena + 24}, {size - 12, size - 32}, {size - 8, size | 3}, {size - 4, arena + 24}}}) 
        c.put32(arena + at, value);
    c.put32(context, arena);
    c.s.allocated[arena] = size;
    c.put32(manager, data + 0x12000);
    c.put32(manager + 4, 3);
    static constexpr std::array<std::uint8_t, 6> kinds{0x13, 0x13, 0x13, 0x14, 0x03, 0x0f};
    const unsigned found = mix.below(3);
    for (unsigned i = 0; i < 3; ++i) {
        const auto entry_at = data + 0x12100 + i * 0x100;
        c.put32(data + 0x12000 + i * 4, entry_at);
        for (unsigned b = 0; b < 0x100; ++b) c.put8(entry_at + b, 0);
        const auto named = marked.empty() ? 7u : marked[(i + mix.below(2)) % marked.size()];
        c.put32(entry_at + 0xc, i == found || mix.below(3) == 0 ? named : named + 200 + i);
        c.put32(entry_at + 0x28, i);
        c.put8(entry_at + 0x34, kinds[mix.below(kinds.size())]);
        c.put8(entry_at + 0x36, mix.below(6));
    }
}

void build(Cpu& c, const Params& p, std::uint32_t entry) {
    Mix mix{p.seed * 2654435761u + 17};
    const auto toc = [&](std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); };
    toc(-0x56bc, floats_at);
    toc(-0x7b40, transfers_cell);
    toc(-0x7b3c, stats_cell);
    toc(-0x7b7c, contracts_cell);
    toc(-0x7b80, calendar_word);
    toc(-0x7be4, count_cell);
    toc(-0x5614, doubles_at);
    c.put32(transfers_cell, manager);
    c.put32(stats_cell, p.head == 4 ? 0u : stats_object);
    c.put32(contracts_cell, data + 0x10d00);
    c.put32(count_cell, 200);
    c.put32(calendar_word, 0x5a5a5a5a);
    c.put_f32(floats_at, 1.0f);
    c.put_f32(floats_at + 0x18, 3.0f);
    for (const auto& [offset, value] : std::array<std::pair<std::uint32_t, double>, 5>{{
             {0x40, 0.0}, {0x120, 1.5}, {0x60, 0.5}, {0x128, 2.0}, {0x18, 4503601774854144.0}}})
        c.put_f64(doubles_at + offset, value);
    const auto ratio = [&] {
        switch (mix.below(5)) {
        case 0: return std::pair{10.0, 0.0};
        case 1: return std::pair{0.0, 0.0};
        case 2: return std::pair{std::numeric_limits<double>::quiet_NaN(), 4.0};
        default: return std::pair{10.0, 4.0};
        }
    }();
    c.put_f64(stats_object + 0x10, ratio.first);
    c.put_f64(stats_object + 0x18, ratio.second);

    // The person: id (small, over the count, or negative), nation, bytes of the id windows, the date words.
    const auto id = mix.pick<std::uint32_t>({std::uint32_t(mix.below(900)), mix.below(900) + 1, 1500, 0x80000000u + mix.below(50), mix.below(30)});
    for (unsigned i = 0; i < 0x70; ++i) c.put8(person_at + i, mix.next());
    c.put32(person_at, id);
    c.put32(person_at + 0x14, record_at);
    c.put32(person_at + 0x18, prefs_at);
    c.put32(person_at + 0xc, mix.below(3));
    const auto signed_id = std::int32_t(id);
    const auto rem6 = signed_id - 6 * (signed_id / 6);
    const auto limit_a = rem6 + 6, limit_b = 15 - rem6;  // byte65 >= limit_a, byte63 <= limit_b pass
    c.put8(person_at + 0x65, std::uint32_t(limit_a + (p.head == 1 ? -1 - std::int32_t(mix.below(2)) : std::int32_t(mix.below(3)) - (mix.below(4) == 0 ? 1 : 0))));
    c.put8(person_at + 0x63, std::uint32_t(limit_b + (p.head == 2 ? 1 + std::int32_t(mix.below(2)) : -std::int32_t(mix.below(3)) + (mix.below(4) == 0 ? 1 : 0))));
    for (unsigned i = 0; i < 0x40; ++i) c.put8(record_at + i, mix.next());
    for (unsigned i = 0; i < 0x40; ++i) c.put8(prefs_at + i, 0);
    // The club: nation, value halfword, the slots.
    for (unsigned i = 0; i < 0x1f0 + 4; ++i) c.put8(club_at + i, mix.next());
    c.put32(club_at + 0x2c, mix.below(3));
    c.put16(club_at + 0x1ec, mix.pick<std::uint32_t>({0x300, 0x1000, 0x1500, 0x2000, 0x2ee, 0x8000}));
    unsigned disliked = 0, favoured = 0;
    std::vector<std::uint32_t> ids;
    const auto candidate_at = [&](unsigned k) { return p.with_high && k % 5 == 2 && k < 15 ? high + k * stride : candidates_at + k * stride; };
    const auto gate = [&](unsigned wanted, unsigned which) { return which == wanted; };
    for (unsigned k = 0; k < 50; ++k) {
        const auto base = candidate_at(k);
        const auto empty = k >= p.slots;
        c.put32(club_at + 0x98 + 4 * k, empty ? (mix.below(8) == 0 ? person_at : 0) : base);
        if (empty) continue;
        if (p.ties && k % 2 == 1) {  // a copy of the candidate before it (record and contract too): the same score
            const auto before = candidate_at(k - 1);
            for (unsigned i = 0; i < 0x140; ++i) c.put8(base + i, c.u8(before + i));
            c.put32(base + 0x14, base + 0x100);
            const auto from = contract_of(before), to = contract_of(base);
            for (unsigned i = 0; i < 0x80; ++i) c.put8(to + i, c.u8(from + i));
            ids.push_back(c.u32(base));
            continue;
        }
        // One failing gate with probability (100 - pass_percent), else none (99).
        const unsigned failing = mix.below(100) < p.pass_percent ? 99 : mix.below(18);
        for (unsigned i = 0; i < 0x80; ++i) c.put8(base + i, mix.next());
        const auto record = base + 0x100;
        for (unsigned i = 0; i < 0x40; ++i) c.put8(record + i, mix.next());
        const auto fail = [&](unsigned which) { return gate(which, failing); };
        // id: below the count, with (id + person id) % 3 != 0 (or == 0 for gate 2), or past the count (gate 1).
        std::uint32_t cid = mix.below(120);
        while (((std::int32_t(cid + id) % 3 == 0 ? 1 : 0) == (fail(2) ? 0 : 1) || std::find(ids.begin(), ids.end(), cid) != ids.end()) && cid < 5000) ++cid;
        if (fail(1)) cid = mix.pick<std::uint32_t>({200, 201, 0x7fffffff});
        c.put32(base, cid);
        ids.push_back(cid);
        c.put32(base + 0x14, fail(0) ? 0u : record);
        c.put32(base + 0xc, mix.below(3));
        // The disliked list holds the candidate (gate 3); the favoured list sometimes.
        if (fail(3) && disliked < 3) c.put32(prefs_at + 0x28 + 4 * disliked++, base);
        if (!p.ties && mix.below(4) == 0 && favoured < 3) c.put32(prefs_at + 0x1c + 4 * favoured++, base);
        // The record's value halfword (+0xa) over 0x109a and over the club value - 0x2ee (gates 4, 5), +4 over 0x64 (6).
        const auto club_value = std::int16_t(c.u16(club_at + 0x1ec));
        const auto above = [&](std::int32_t limit, bool failing_value) {
            const auto delta = std::int32_t(mix.below(4));
            return std::uint32_t(failing_value ? limit - delta : limit + 1 + delta);
        };
        const auto floor_value = std::max<std::int32_t>(0x109a, club_value - 0x2ee);
        c.put16(record + 0xa, fail(4) ? above(0x109a, true) : fail(5) ? above(club_value - 0x2ee, true) : above(floor_value, false));
        c.put16(record + 0x4, fail(6) ? mix.pick<std::uint32_t>({0x64, 0x63, 0, 0x8000}) : mix.pick<std::uint32_t>({0x65, 0x66, 500, 0x7fff}));
        // The kind byte (+0x66): below 0xb tests the position masks; the ratings decide whether they match.
        c.put8(base + 0x66, mix.pick<std::uint32_t>({0, 3, 10, 11, 12, 0x80, 0xff, 5}));
        for (unsigned r = 0xf; r <= 0x19; ++r) c.put8(record + r, mix.pick<std::uint32_t>({0, 10, 11, 14, 15, 19, 20, 20, 0x94}));
        if (fail(7)) {  // the same mask as the person's: copy the person's ratings, kind below 0xb
            c.put8(base + 0x66, mix.below(11));
            for (unsigned r = 0xf; r <= 0x19; ++r) c.put8(record + r, c.u8(record_at + r));
        }
        c.put8(base + 0x67, mix.pick<std::uint32_t>({0, 5, 6, 7, 15, 16, 0x7f, 0x80, 0xff}));
        c.put8(record + 0x21, mix.pick<std::uint32_t>({0x14, 15, 16, 0x15, 0, 0xff, 0x80, 0x13}));
        const auto contract = contract_of(base);
        for (unsigned i = 0; i < 0x80; ++i) c.put8(contract + i, mix.next());
        c.put32(contract + 0x50, mix.pick<std::uint32_t>({0, 0x20, 0x20000, 0x20020, 0x10, 0x40000, 0xffffffffu}));
        c.put8(contract + 0x60, mix.next());
    }
    c.put16(person_at + 0x5c, mix.pick<std::uint32_t>({0, 0x17, 0x18, 0x7f, 0x80, 0xff17, 0x1218}));
    // The second routine's candidate.
    std::uint32_t chosen = 0;
    if (entry == 0x100dbc20) {
        switch (p.slots == 0 && p.x_kind == 0 ? 1u : p.x_kind) {
        case 0: {
            const auto k = std::min(p.slots, 49u) == 0 ? 0 : mix.below(std::min(p.slots, 49u));
            chosen = candidate_at(p.ties ? k - k % 2 : k);
            break;
        }
        case 1: {
            chosen = fresh_at;
            for (unsigned i = 0; i < 0x180; ++i) c.put8(chosen + i, mix.next());
            c.put32(chosen, 40 + mix.below(80));
            ids.push_back(c.u32(chosen));
            c.put32(chosen + 0x14, chosen + 0x100);
            c.put32(chosen + 0xc, mix.below(3));
            c.put16(chosen + 0x100 + 0xa, 0x2000 + mix.below(3000));
            c.put16(chosen + 0x100 + 0x4, 0x100);
            c.put8(chosen + 0x67, mix.below(20));
            c.put32(contract_of(chosen), 0);
            for (unsigned i = 0; i < 0x80; ++i) c.put8(contract_of(chosen) + i, mix.next());
            break;
        }
        case 2: chosen = person_at; break;
        default: chosen = 0; break;
        }
    }
    transfer_world(c, p, mix, ids);
    c.set(3, p.null_person ? 0u : person_at);
    c.set(4, p.null_club ? 0u : club_at);
    c.set(5, chosen);
}
}

std::vector<Fixture> people_staff_fixtures() {
    std::vector<Fixture> found;
    Mix choose{0x57a9f};
    unsigned serial = 0;
    for (const std::uint32_t entry : {0x100dc4d0u, 0x100dbc20u}) {
        const auto add = [&](const Params& p) {
            auto f = people_checks::make(entry, "staff seed " + std::to_string(p.seed) + " pass " + std::to_string(p.pass_percent) +
                                                    " slots " + std::to_string(p.slots) + " head " + std::to_string(p.head) +
                                                    " x " + std::to_string(p.x_kind) + (p.null_person ? " null-person" : "") +
                                                    (p.null_club ? " null-club" : "") + (p.with_high ? " high" : ""),
                                         Draws{}, [p, entry](Cpu& c) { build(c, p, entry); }, p.overflow);
            f.stub = [p](State& s) { return staff_stub(s, p); };
            found.push_back(std::move(f));
        };
        // Early outs and the head's windows.
        for (const bool null_person : {false, true})
            for (const bool null_club : {false, true})
                add({.seed = ++serial, .pass_percent = 90, .slots = 3, .contract_percent = 90, .transfer_percent = 90,
                     .head = 0, .null_person = null_person, .null_club = null_club});
        for (unsigned head = 1; head <= 4; ++head)
            for (unsigned seed = 0; seed < 6; ++seed)
                add({.seed = ++serial, .pass_percent = 90, .slots = 4, .contract_percent = 90, .transfer_percent = 90, .head = head,
                     .x_kind = seed % 4});
        // The candidates.
        for (unsigned round = 0; round < 700; ++round) {
            Params p{.seed = ++serial,
                     .pass_percent = choose.pick<unsigned>({100, 97, 90, 80, 60, 30}),
                     .slots = choose.pick<unsigned>({0, 1, 2, 3, 5, 8, 20, 50, 50}),
                     .contract_percent = choose.pick<unsigned>({100, 95, 70, 20}),
                     .transfer_percent = choose.pick<unsigned>({100, 90, 60, 10}),
                     .head = 0};
            p.with_high = choose.below(6) == 0;
            p.x_kind = choose.pick<unsigned>({0, 0, 0, 1, 2, 3});
            p.overflow = choose.below(4) == 0;
            add(p);
        }
        for (unsigned round = 0; round < 60; ++round) {  // equal scores: the first keeps the slot, nothing is better
            Params p{.seed = ++serial, .pass_percent = 100, .slots = choose.pick<unsigned>({2, 4, 10}), .contract_percent = 100,
                     .transfer_percent = 100, .head = 0};
            p.ties = true;
            p.x_kind = 0;
            add(p);
        }
    }
    return found;
}
}
