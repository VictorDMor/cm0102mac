// cm-database-checks fixtures for DATABASE.CPP's init block (database_checks.hpp): Player Initialisation
// (0x100b8cf0) over small seeded worlds. The sample pool is scripted, other modules' calls are stubbed (their
// arguments and the preserved registers are recorded), and DATABASE.CPP's own callees run for real.
#include "database_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <set>

namespace cm::aot::database_checks {
namespace {
using namespace cm::aot::guest;
constexpr std::uint32_t entry = 0x100b8cf0;
constexpr std::uint32_t count_word = data + 0x100, table_word = data + 0x104, pointer_word = data + 0x108,
                        end_word = data + 0x10c, salt_word = data + 0x110, nation_word = data + 0x114,
                        nation_count_word = data + 0x118, index_word = data + 0x11c, lookup_word = data + 0x120,
                        pool = data + 0x4000, persons = data + 0x8000, nations = data + 0x9000, clubs = data + 0xa000,
                        players = data + 0xb000, staffs = data + 0xd000, indexes = data + 0xf000,
                        halfwords = data + 0x1400, date_arg = data + 0x2500, id_words = data + 0x130;
constexpr std::uint32_t screen_object = data + 0x11000;
constexpr unsigned pool_size = 3000;

struct World {
    std::uint32_t seed = 1;
    unsigned people = 3;
    bool wide_count = false;     // a nation count above 65535 (the wide draw)
    bool tiny_pool = false;      // the pool wraps (refills) often
    bool all_staff = false;      // no player records
    bool unset_names = false;
    bool valid = false;          // every attribute byte of the player and staff records within 1-20
    bool wide_people = false;    // a person count of 130000 while the closest-record searches run (a shifted table maps it)
    bool spill = false;          // a count of 300 (a progress step of 3) over 3 rows whose nations print the assert
    bool clamp = false;          // the wide draw reaches its bound (scripted pool, nation table shifted to fit)
};

void toc_word(Cpu& c, std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); }

void build(Cpu& c, const World& w) {
    std::mt19937 rng(w.seed);
    const auto pick = [&](std::initializer_list<std::int32_t> values) {
        return *(values.begin() + rng() % values.size());
    };
    // Random bytes of every record, biased to 1-20 and the sentinels.
    for (std::uint32_t a = persons; a < indexes + 0x1000; ++a) {
        const auto r = rng() % 8;
        if (w.valid && ((a >= players && a < players + 0x1000) || (a >= staffs && a < staffs + 0x1000))) {
            c.put8(a, 1 + rng() % 20);
            continue;
        }
        c.put8(a, r < 5 ? 1 + rng() % 20 : r == 5 ? 0 : r == 6 ? 0xff : r == 7 && rng() % 2 ? 15 + rng() % 2 : rng());
    }
    toc_word(c, -0x7be4, count_word);
    toc_word(c, -0x7be0, table_word);
    c.put32(table_word, persons);
    c.put32(count_word, w.spill ? 300 : w.people);
    toc_word(c, -0x7b84, pointer_word);
    toc_word(c, -0x7b88, end_word);
    toc_word(c, -0x7b8c, pool);
    toc_word(c, -0x7b90, salt_word);
    const unsigned size = w.tiny_pool ? 40 : pool_size;
    for (unsigned i = 0; i < pool_size; ++i) {
        const auto r = rng() % 6;
        c.put16(pool + 2 * i, w.wide_people ? 0xfffe : w.wide_count ? rng() % 15 : r == 0 ? 0 : r == 1 ? 0xffff : r == 2 ? rng() % 200 * 2 : rng());
    }
    c.put32(pointer_word, pool - 2);
    c.put32(end_word, pool + 2 * (size - 1));
    c.put32(salt_word, rng() % 3 == 0 || w.wide_count || w.wide_people ? 0 : rng() & 0xffff);
    toc_word(c, -0x34c4, rng());
    toc_word(c, -0x7b98, nation_word);
    c.put32(nation_word, nations);
    toc_word(c, -0x7b04, nation_count_word);
    c.put32(nation_count_word, w.wide_count ? 70000 : 3);
    toc_word(c, -0x7b9c, index_word);
    c.put32(index_word, indexes);
    toc_word(c, -0x7a70, lookup_word);
    c.put32(lookup_word, data + 0x124);
    c.put32(data + 0x124, 77);
    const std::int32_t nation_ids[]{-0x7a14, -0x7574, -0x7a10, -0x7a1c, -0x7578, -0x757c, -0x7504};
    for (unsigned i = 0; i < 7; ++i) {
        toc_word(c, nation_ids[i], id_words + 4 * i);
        c.put32(id_words + 4 * i, 10 + i);
    }
    // The name tables the pickers read: rows of zero (a count of 0: one draw below the bound word).
    constexpr std::uint32_t zero_rows = data + 0x15000, words = data + 0x140;
    for (std::uint32_t a = zero_rows; a < zero_rows + 0x2800; ++a) c.put8(a, 0);
    unsigned slot = 0;
    for (const std::int32_t offset : {-0x7580, -0x752c, -0x7530, -0x758c, -0x7590, -0x7534}) {
        toc_word(c, offset, words + 4 * slot);
        c.put32(words + 4 * slot++, zero_rows);
    }
    for (const std::int32_t offset : {-0x754c, -0x7550, -0x7554}) {
        toc_word(c, offset, words + 4 * slot);
        c.put32(words + 4 * slot++, 5);
    }
    toc_word(c, -0x7bb8, data + 0x2000);
    toc_word(c, -0x7bb0, screen_object);
    for (std::uint32_t a = screen_object; a < screen_object + 0x3000; ++a) c.put8(a, 0);
    c.put32(screen_object + 0x2c2c, data + 0x14000);  // the packet's buffer and capacity
    c.put32(screen_object + 0x2c34, 0x800);
    c.put8(data + 0x2000, 0);
    toc_word(c, -0x7bac, data + 0x2200);
    toc_word(c, -0x7ba8, data + 0x2300);
    toc_word(c, -0x7a9c, data + 0x2400);
    toc_word(c, -0x5634, halfwords);
    for (int i = -128; i < 128; ++i) c.put16(halfwords + 2 * i, rng() % 90);
    // The reference date.
    c.put16(date_arg, 1 + rng() % 28);
    c.put16(date_arg + 2, 0x7d2);
    c.put8(date_arg + 4, 0);
    // Nations: id (+0x68), category (+0x118), +0xac.
    for (unsigned n = 0; n < 4; ++n) {
        const auto nation = nations + n * 0x128;
        c.put32(nation + 0x68, 10 + rng() % 9);
        c.put32(nation + 0x74, data + 0x2600);
        c.put32(data + 0x2608, 77);
        c.put8(nation + 0x118, 4 + rng() % 10 + (rng() % 5 == 0 ? 20 : 0));
        c.put16(nation + 0xac, pick({0, 256, 3, -5, 7}));
    }
    for (unsigned k = 0; k < 4; ++k) {
        const auto club = clubs + k * 0x248;
        c.put32(club + 0x2c, rng() % 5 ? nations + rng() % 4 * 0x128 : 0);
        c.put16(club + 0x1ec, pick({0, 100, 5000, 6251, 7750, 7751, 9000, -3, 20000}));
    }
    for (unsigned i = 0; i < w.people; ++i) {
        const auto p = persons + i * 0x70;
        c.put32(p + 0, i);
        c.put32(p + 0x14, !w.all_staff && rng() % 6 ? players + i * 0x200 : 0);
        c.put32(p + 0x1c, rng() % 5 ? staffs + i * 0x200 : 0);
        c.put32(p + 0xc, rng() % 3 ? nations + rng() % 4 * 0x128 : 0);
        c.put32(p + 0x20, rng() % 3 ? clubs + rng() % 4 * 0x248 : 0);
        c.put32(p + 0x24, rng() % 3 ? clubs + rng() % 4 * 0x248 : 0);
        for (const auto name : {0x28u, 0x2cu, 0x30u}) c.put32(p + name, w.unset_names && rng() % 2 ? 0xfffffffe : 0x60001000 + i);
        c.put16(p + 0x4c, pick({1, 2, 28, -1, 5, 6, 7}));
        c.put16(p + 0x4e, pick({0x7b0, 0x7b1, 0x7b2, 0x7cc, 0x7d0, 0x700, 0x7a0, 0x7b5, 0x7b6, 0x7b7, 0x7b8, 0x7b9}));
        c.put16(p + 0x54, pick({1, 31, 5}));
        c.put16(p + 0x56, pick({0x76c, 0x7cf, 0x7d0, 0x7da, 0x7db, 0x7ce}));
        c.put16(p + 0x44, pick({1, 31, 5}));
        c.put16(p + 0x46, pick({0x76c, 0x7cf, 0x7d0, 0x7da, 0x7db, 0x7ce}));
        c.put16(p + 0x5c, pick({0x76d, 0x780, 0x7ae, 0x7af, 0x7b0, 0x10, 30, 25, 60}));
        c.put8(p + 0x5e, rng() % 2);
        c.put8(p + 0x60, pick({0, 0, 3}));
        c.put8(p + 0x62, pick({0, 0, 5, 8, 9, 10, 4, 11}));
        for (unsigned r = 0; r < 9; ++r)
            if (r != 1) c.put8(p + 0x61 + r, w.valid ? 1 + rng() % 20 : pick({-1, 0, 5, 15, 21, 100, 3, 20}));
        const auto player = players + i * 0x200, staff = staffs + i * 0x200;
        for (const auto offset : {4u, 6u, 8u, 0xau, 0xcu, 0x24u, 0x26u, 0x28u, 0x2au, 0x2cu})
            if (!w.valid || offset < 0x24) c.put16(player + offset, pick({-2, -1, 0, 1, 5, 50, 99, 100, 150, 199, 200, 201, 5000}));
        if (w.valid)
            for (const auto offset : {4u, 6u, 8u, 0xau, 0xcu}) c.put16(player + offset, 1 + rng() % 20);
        c.put8(player + 0xf, pick({20, 20, 14}));
        if (w.valid) c.put8(player + 0xf, 20);  // (+0x2e stays a drawn 1-20 byte)
        else if (w.seed % 3 == 0) c.put8(player + 0xf, std::array<std::uint8_t, 6>{13, 14, 15, 16, 15, 17}[(w.seed / 3 + i) % 6]);
        c.put8(player + 0x2e, pick({3, 7, 9}));
        for (const auto offset : {0x2cu, 0x2au, 0x28u, 0x24u, 0x26u})
            c.put16(staff + offset, pick({-1, 0, 1, 5, 50, 150, 201, 2000, 20000}));
        for (const auto offset : {0x2fu, 0x30u, 0x31u, 0x32u, 0x33u, 0x37u, 0x38u, 0x3au, 0x3bu, 0x3eu, 0x3fu, 0x41u, 0x43u})
            c.put8(staff + offset, pick({0, 1, 3, 6, 9, 12, 15, 25, 0xff}));
    }
    if (w.wide_people) {  // person 0 has both records, in range, for the searches to find
        c.put16(players + 4, 50);
        c.put16(staffs + 0x24, 50);
        c.put32(persons + 0x14, players);
        c.put32(persons + 0x1c, staffs);
    }
    if (w.spill) {  // every nation prints the assert, every row has a player
        for (unsigned n = 0; n < 4; ++n) c.put16(nations + n * 0x128 + 0xac, 0);
        for (unsigned i = 0; i < w.people; ++i) {
            c.put32(persons + i * 0x70 + 0xc, nations + i % 4 * 0x128);
            c.put32(persons + i * 0x70 + 0x14, players + i * 0x200);
            c.put32(persons + i * 0x70 + 0x1c, staffs + i * 0x200);
        }
    }
    if (w.clamp) {
        // Person 0 draws its nation from a count of 130000: the sample 0xfffe scales to 129998 and the
        // bias drawn next (15 of 19) adds 2, past 129999 and clamped to it, which the shifted base maps.
        c.put32(nation_count_word, 130000);
        c.put32(nation_word, nations - 129999 * 0x128);
        c.put16(pool, 0xfffe);
        c.put16(pool + 2, 15);
        c.put32(salt_word, 0);
        for (const auto offset : {0x61u, 0x63u, 0x64u, 0x65u, 0x66u, 0x67u, 0x68u, 0x69u}) c.put8(persons + offset, 10);
    }
    if (w.wide_count || w.clamp) {  // person 0 draws its nation from the wide count
        c.put32(persons + 0xc, 0);
        c.put32(persons + 0x20, 0);
        c.put32(persons + 0x24, 0);
    }
    c.set(3, date_arg);
}

const std::uint32_t stubbed[]{0x10739890, 0x10768810, 0x10769b60, 0x1077b720, 0x1076f7d0, 0x10727890, 0x10743120,
                              0x107158a0, 0x107156b0, 0x10717cb0, 0x1036ff00};

bool stub(State& s, const World& w) {
    bool known = false;
    for (const auto target : stubbed) known |= target == s.pc;
    if (!known) return false;
    Cpu c(s);
    const auto calls = c.u32(trace_at);
    const auto target = s.pc;
    // The call, its link register, the argument registers it reads (a varargs call all of r3-r10) and the
    // preserved registers and condition fields: the leftovers of other volatile registers are not recorded.
    {
        const auto size = c.u32(trace_at);
        auto at = trace_at + 4 + size * 4;
        const auto add = [&](std::uint32_t value) { c.put32(at, value); at += 4; };
        add(s.pc);
        add(c.lr());
        unsigned last = 5;
        if (target == 0x107156b0 || target == 0x10717cb0 || target == 0x10727890 || target == 0x10769b60) last = 5;
        else if (target == 0x1036ff00) last = 4;
        else if (target == 0x1076f7d0) last = 3;
        else if (target == 0x1077b720) last = 10;
        else if (target == 0x107158a0 || target == 0x10743120 || target == 0x10739890) last = 7;
        else if (target == 0x10768810) last = 2;
        for (unsigned r = 3; r <= last; ++r) add(c.r(r));
        for (unsigned r = 13; r < 32; ++r) add(c.r(r));
        for (unsigned f = 1; f < 8; ++f) add(c.cr(f));  // cr0 is a leftover compare
        c.put32(trace_at, (at - trace_at - 4) / 4);
    }
    if (target == 0x107156b0) {
        c.put16(c.r(3), c.r(4));
        c.put16(c.r(3) + 2, c.r(5));
        c.put8(c.r(3) + 4, 0);
    } else if (target == 0x10717cb0) {
        c.put16(c.r(3), 5 + calls % 3);
        c.put16(c.r(3) + 2, 0x7b0 + c.r(5) + calls % 7);
        c.put32(c.r(3) + 4, 0x01020300 + calls);
    } else if (target == 0x1036ff00) {
        c.set(3, (calls * 7 + c.r(4)) % 5 == 0);
    } else {
        c.set(3, calls);
    }
    // Worlds that change the person count or table while a row runs (both sides see the same stub).
    if (target == 0x10717cb0 && w.wide_people && c.u32(count_word) == 1) {
        // The closest-record searches draw below 130000: a table shifted so that index 129998 (the draw's
        // value for the sample 0xfffe) is person 0.
        c.put32(count_word, 130000);
        c.put32(table_word, persons - 129998 * 0x70);
    } else if (target == 0x1036ff00 && w.wide_people && c.u32(count_word) == 130000) {
        c.put32(count_word, 1);
        c.put32(table_word, persons);
    } else if (target == 0x10717cb0 && w.spill && c.u32(count_word) == 300) {
        c.put32(count_word, w.people);  // the progress step was taken from 300; the rows are the real ones
    }
    s.pc = c.lr();
    return true;
}
}

std::vector<Fixture> init_fixtures() {
    std::vector<Fixture> found;
    const auto add = [&](const World& w, const std::string& description) {
        Fixture f;
        f.entry = entry;
        f.description = description;
        f.setup = [w](Cpu& c) { build(c, w); };
        f.stub = [w](State& s) { return stub(s, w); };
        f.results = 0;
        found.push_back(std::move(f));
    };
    // Seeds whose draws fault on both sides (the checker does not accept a fault as a comparison) are left out;
    // unset names run the pickers over zero tables, so some seeds keep their names.
    static const std::set<std::uint32_t> faulting{0, 13, 26, 39, 52, 65, 78, 91, 104, 117, 143, 156, 169, 221, 234, 247, 260, 273, 299, 312, 325, 338, 364, 390, 403, 416, 429, 442, 494, 507, 533, 546, 559, 585, 637, 650, 663, 676, 689, 728, 741, 754, 767, 793};
    static const std::set<std::uint32_t> both_fault{0, 2002, 2233, 2464};
    for (std::uint32_t seed = 1; seed <= 2500; ++seed) {
        if (both_fault.contains(seed)) continue;
        World w;
        w.seed = seed;
        w.people = 1 + seed % 5;
        w.tiny_pool = seed % 7 == 0;
        w.all_staff = seed % 9 == 0;
        w.unset_names = seed <= 800 && seed % 13 == 0 && !faulting.contains(seed);
        w.wide_count = seed % 11 == 0;
        add(w, "seed " + std::to_string(seed));
    }
    for (std::uint32_t seed = 3001; seed <= 3020; ++seed) {  // the wide draw's clamp to bound - 1
        if (seed == 3005 || seed == 3017) continue;
        World w;
        w.seed = seed;
        w.people = 1 + seed % 3;
        w.clamp = true;
        w.wide_count = true;
        add(w, "clamp seed " + std::to_string(seed));
    }
    for (std::uint32_t seed = 4001; seed <= 4400; ++seed) {  // every attribute byte within 1-20 (the "already valid" paths)
        World w;
        w.seed = seed;
        w.people = 1 + seed % 4;
        w.valid = true;
        add(w, "valid seed " + std::to_string(seed));
    }
    for (std::uint32_t seed = 5001; seed <= 5020; ++seed) {  // the closest-record searches over a count of 130000
        if (seed == 5007 || seed == 5016) continue;  // a fault on both sides
        World w;
        w.seed = seed;
        w.people = 1;
        w.wide_people = true;
        add(w, "wide people seed " + std::to_string(seed));
    }
    for (std::uint32_t seed = 6001; seed <= 6030; ++seed) {  // rows without a progress message after a staff row
        World w;
        w.seed = seed;
        w.people = 3;
        w.spill = true;
        add(w, "spill seed " + std::to_string(seed));
    }
    return found;
}
}
