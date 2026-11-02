// Cases for the press item 0xbdd (0x1067e210): a person, contract and owner built in the scratch area (so every
// condition can be set), real clubs, the callees that steer the routine answered, and the random table scripted.
// The guards and the tail's branches are listed; the kind choice is explored with seeded random states, since the
// draws and the rating comparisons interact.
#include "guest_abi.hpp"
#include "media_checks.hpp"
#include <random>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
constexpr std::uint32_t entry_bdd = 0x1067e210;
constexpr std::uint32_t table_at = scratch_base + 0x8000, person_at = scratch_base + 0x1000, contract_at = scratch_base + 0x1200,
    owner_at = scratch_base + 0x1400, links_at = scratch_base + 0x1600, standing_at = scratch_base + 0x2000,
    extra_at = scratch_base + 0x2200, staff_at = scratch_base + 0x2400, found_at = scratch_base + 0x2600,
    chain_at = scratch_base + 0x2800, word_at = scratch_base + 0x2a00, special_at = scratch_base + 0x2c00,
    clubs_at = scratch_base + 0x20000;
// The callees that steer the routine (answered, no body run).
constexpr std::uint32_t special_club = 0x100c7d00, standing_of = 0x10082f90, busy = 0x103eaea0, league = 0x102af680,
    distance = 0x10718130, known = 0x1069e700, staff_pick = 0x10647730, seller = 0x106584d0, form = 0x100d71a0,
    coach_of = 0x102b7450, found_of = 0x103f43b0, chain_of = 0x103eb440, roll_of = 0x1069eb10,
    sale = 0x106a95c0, people_of = 0x103fc710, open_club = 0x1014c2e0, follow_up = 0x10677d60, owner_hook = 0x100d9960;

Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
void append(Writes& out, const Writes& more) { out.insert(out.end(), more.begin(), more.end()); }
// The random table as the game keeps it (see the squad cases): the first draw reads entries[0]; `refill` puts the cursor at
// the end so the next step wraps to the start.
Writes random_table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, int wrap_after = -1) {
    Writes writes;
    if (wrap_after >= 0) {
        // The cursor is `wrap_after` draws from the end, so the next draw wraps (and refills the seed with rand()).
        const auto end = table_at + 0x100, cursor = end - 2 * std::uint32_t(wrap_after);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            writes.push_back({2, table_at + 2 * std::uint32_t(i + 1), entries[i]});
            if (i < std::size_t(wrap_after)) writes.push_back({2, cursor + 2 * std::uint32_t(i + 1), entries[i]});
        }
        writes.push_back({2, table_at, 7});
        writes.push_back({4, s.toc(-0x7b84), cursor});
        writes.push_back({4, s.toc(-0x7b88), end});
        writes.push_back({4, s.toc(-0x7b8c), table_at});
        writes.push_back({4, s.toc(-0x7b90), seed});
        return writes;
    }
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back({2, table_at + 2 * std::uint32_t(i + 1), entries[i]});
    const auto end = table_at + 0x100;
    writes.push_back({4, s.toc(-0x7b84), table_at});
    writes.push_back({4, s.toc(-0x7b88), end});
    writes.push_back({4, s.toc(-0x7b8c), table_at});
    writes.push_back({4, s.toc(-0x7b90), seed});
    return writes;
}
// Every knob of a case.
struct Story {
    std::int16_t age = 0x24, contract_a = 0x2000, contract_8 = 0x1000, contract_c = 0x1000, contract_4 = 0x400, contract_10 = 0x2000;
    std::uint8_t byte_5e = 0, grade = 5, byte_67 = 5, region = 0, byte_2f = 0x10;
    std::uint32_t word_20 = 1, word_24 = 0;
    std::int16_t owner = 0x1000, club_rating = 0x2000;
    std::uint32_t person_id = 9;
    std::int16_t date_day = 100, date_year = 2001, today_day = 50, today_year = 2001;
    std::uint32_t flags = 0, extra_pair = 0x03050607;
    // answers
    std::uint32_t is_special = 1, busy_answer = 0, league_answer = 0, distance_answer = 0x500, known_answer = 0, staff_answer = 1,
        seller_answer = 3, found_answer = 0, open_answer = 1, people_count = 3, coach_answer = 1, form_answer = 7, half_answer = 5, sale_answer = 9;
    std::uint32_t club_count = 40;
    std::uint32_t extra = 0, kind = 0x20;
    std::uint32_t staff_20 = 1, staff_24 = 1;
    std::uint8_t staff_60 = 0x41, staff_62 = 0x42;
    std::vector<std::uint32_t> table = {0, 0, 0, 0, 0, 0, 0, 0};
    std::uint32_t seed = 0;
    int wrap = -1;
    std::uint32_t same_id = 0;
    std::uint32_t special_ptr = 0;  // club+0x30
};
}

MEDIA_CASES("pick_b_bdd") {
    // Real clubs with a manager, an owner and a link at +0x30.
    std::vector<std::uint32_t> clubs;
    for (unsigned i = 0; clubs.size() < 3 && i < 400; ++i) {
        const auto c = scenario.club(i);
        const auto manager = scenario.r32(c + 0x90);
        if (manager && scenario.r32(c + 0x2c) && scenario.r32(c + 0x30) && scenario.r32(manager + 0x28) && scenario.r32(manager + 0x2c) && scenario.r32(manager + 0x30))
            clubs.push_back(c);
    }
    const auto sp = std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(1)));
    const auto frame = sp - 0x240;
    const auto today = scenario.toc(-0x7b80);
    const auto press = scenario.global(-0x7b94);
    const auto real_people = scenario.global(-0x7be0);

    const auto build = [&](const Story& st, std::uint32_t club, Registers registers, const char* label, Answers extra_answers = {}) {
        Writes m;
        // The person: id, owner, contract, the links and the ratings.
        append(m, {w32(person_at, st.person_id), w32(person_at + 0xc, owner_at), w32(person_at + 0x14, contract_at), w32(person_at + 0x20, st.word_20),
                   w32(person_at + 0x24, st.word_24 ? links_at + 0x100 : 0), w32(person_at + 0x28, links_at), w32(person_at + 0x2c, links_at + 0x40),
                   w32(person_at + 0x30, links_at + 0x80), h16(person_at + 0x4c, st.date_day), h16(person_at + 0x4e, st.date_year), b8(person_at + 0x50, 0),
                   h16(person_at + 0x5c, st.age), b8(person_at + 0x5e, st.byte_5e), b8(person_at + 0x63, st.grade), b8(person_at + 0x67, st.byte_67),
                   w32(links_at + 0x20, 0x11), w32(links_at + 0x60, 0x22), w32(links_at + 0xa0, 0x33), w32(links_at + 0x100, 0x44), w32(links_at + 0x120, 0x55),
                   h16(contract_at + 4, st.contract_4), h16(contract_at + 8, st.contract_8), h16(contract_at + 0xa, st.contract_a), h16(contract_at + 0xc, st.contract_c),
                   b8(contract_at + 0xf, st.region), b8(contract_at + 0x2f, st.byte_2f),
                   h16(owner_at + 0xac, st.owner), w32(owner_at + 0x68, 0x77),
                   w32(standing_at + 0x50, st.flags), w32(standing_at + 0x44, 0),
                   w32(extra_at, 0x1234), w32(extra_at + 0x1c, st.extra_pair & 1 ? club : clubs[1]), w32(extra_at + 0x20, st.extra_pair & 2 ? club : clubs[2]),
                   b8(extra_at + 0x49, st.extra_pair >> 24), b8(extra_at + 0x4a, (st.extra_pair >> 16) & 0xff),
                   w32(staff_at, 0x4321), w32(staff_at + 0x20, st.staff_20 ? links_at + 0x100 : 0), w32(staff_at + 0x24, st.staff_24 ? links_at + 0x120 : 0),
                   w32(staff_at + 0x28, links_at), w32(staff_at + 0x2c, links_at + 0x40), w32(staff_at + 0x30, links_at + 0x80),
                   b8(staff_at + 0x60, st.staff_60), b8(staff_at + 0x62, st.staff_62),
                   h16(found_at, 0x1234), h16(found_at + 2, 0x4567), w32(found_at + 8, 0x88),
                   w32(chain_at, chain_at + 0x40), w32(chain_at + 0x48, 0x99), w32(word_at, 0xabc),
                   w32(frame + 0x44, found_at),
                   w32(club + 0x30, st.special_ptr ? st.special_ptr : special_at), w32(special_at, 5), w32(scenario.toc(-0x7a0c), st.same_id ? 5 : 6),
                   w32(scenario.toc(-0x7b50), st.club_count)});
        for (unsigned i = 0; i < 30; ++i) m.push_back(w32(frame + 0x7c + 4 * i, real_people + (i % 7 + 1) * 0x70));
        append(m, {h16(today, st.today_day), h16(today + 2, st.today_year), w32(today + 4, 0)});
        m.push_back(h16(club + 0x1ec, st.club_rating));
        append(m, random_table(scenario, st.table, st.seed, st.wrap));
        Answers answers{{special_club, st.is_special}, {standing_of, standing_at}, {busy, st.busy_answer}, {league, st.league_answer}, {distance, st.distance_answer},
                        {known, st.known_answer}, {staff_pick, st.staff_answer ? staff_at : 0}, {seller, st.seller_answer}, {found_of, st.found_answer}, {chain_of, chain_at},
                        {people_of, st.people_count}, {open_club, st.open_answer}, {coach_of, st.coach_answer ? word_at : 0}, {form, st.form_answer},
                        {roll_of, 3}, {sale, st.sale_answer}, {follow_up, 0}, {owner_hook, 0}};
        for (const auto& a : extra_answers) {
            bool replaced = false;
            for (auto& existing : answers) if (existing.first == a.first) { existing.second = a.second; replaced = true; }
            if (!replaced) answers.push_back(a);
        }
        cases.push_back({.entry = entry_bdd, .registers = std::move(registers), .memory = std::move(m), .f1 = {}, .label = label, .answers = std::move(answers)});
    };
    const auto regs = [&](std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t extra) -> Registers {
        return {{3, press}, {4, person}, {5, club}, {6, kind}, {7, extra}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}};
    };
    const Story base;
    const auto club = clubs[0];

    // The guards.
    build(base, club, regs(0, club, 0x20, 0), "no person");
    { Story s = base; build(s, club, regs(person_at, club, 0x20, 0), "guards ok"); }
    build(base, club, regs(person_at, 0, 0x20, 0), "no club");
    {
        // no contract, low rating, no owner (person+0xc), no manager, no club owner, special club cases.
        Story s = base;
        const auto call = [&](Writes more, std::uint32_t at, const char* label, Answers a = {}) {
            build(s, at, regs(person_at, at, 0x20, 0), label, a);
            append(cases.back().memory, more);
        };
        call({w32(person_at + 0x14, 0)}, club, "no contract");
        for (const std::int16_t rating : {0xdab, 0xdac, 0x7fff, -1}) call({h16(contract_at + 0xa, std::uint16_t(rating))}, club, "rating");
        call({w32(person_at + 0xc, 0)}, club, "no owner");
        call({w32(club + 0x90, 0)}, club, "no manager");
        call({w32(club + 0x2c, 0)}, club, "no club owner");
        s.is_special = 0;
        call({}, club, "special 0");
        call({w32(club + 0x30, 0)}, club, "special 0, no link");
        { Story same = s; same.same_id = 1; build(same, club, regs(person_at, club, 0x20, 0), "special 0, same id"); }
        call({}, club, "special 0, no standing", {{standing_of, 0}});
        for (const std::uint32_t flags : {0u, 8u, 0x2000u, 0x2008u}) { s.flags = flags; call({}, club, "special 0, flags"); }
    }

    // The tail for the kinds 0x33 and 0x34 (no tree): the extra record, the coach lookups, the found record, the staff.
    for (const std::uint32_t kind : {0x33u, 0x34u, 0x20u, 0x1u, 0x80u + 0x33u})
        for (const std::uint32_t extra : {0u, extra_at})
            for (const std::uint32_t coach : {0u, 1u})
                for (const std::uint32_t found : {0u, 1u})
                    for (const std::uint32_t staff : {0u, 1u})
                        for (const std::uint32_t open : {0u, 1u}) {
                            Story s = base;
                            s.coach_answer = coach; s.found_answer = found; s.staff_answer = staff; s.open_answer = open;
                            s.table = {1, 2, 3, 4, 5, 6};
                            build(s, club, regs(person_at, club, kind, extra), "tail");
                        }
    // The extra record's sides and the staff record's variants.
    for (const std::uint32_t pair : {0x03030000u, 0xff030000u, 0x05030001u, 0x05030002u, 0x05030003u, 0x03050001u, 0x03050002u, 0x03050003u, 0x03050000u})
        for (const std::uint32_t staff_variant : {0u, 1u, 2u, 3u}) {
            Story s = base;
            s.extra_pair = pair;
            s.staff_20 = staff_variant & 1; s.staff_24 = (staff_variant >> 1) & 1;
            s.word_24 = staff_variant & 1;
            s.table = {1, 2, 3, 4, 5, 6};
            build(s, club, regs(person_at, club, 0x34, extra_at), "sides");
        }
    for (const std::uint32_t seller_value : {0u, 1u, 0x80u}) for (const std::uint32_t people : {0u, 1u, 3u, 0x80u, 0x1ffu}) {
        Story s = base;
        s.seller_answer = seller_value; s.people_count = people;
        s.table = {1, 2, 3, 4, 5, 6};
        build(s, club, regs(person_at, club, 0x20, 0), "people");
        build(s, club, regs(person_at, club, 0x33, 0), "people 33");
        s.club_rating = 0x2000; s.contract_c = 0x1700;
        build(s, club, regs(person_at, club, 0x33, 0), "people 33 rich");
    }
    for (const std::int16_t club_rating : {0x100, 0xfa0, 0xfa1, -5}) for (const std::uint8_t flag_byte : {0, 0x14}) {
        Story s = base;
        s.club_rating = club_rating;
        build(s, club, regs(person_at, club, 0x34, 0), "ratings");
        append(cases.back().memory, {b8(contract_at + 0xf, flag_byte)});
    }
    // Rating caps after the story.
    for (const std::int16_t a : {0x2600, 0x2650, 0x7f00, 0x2710, 0x1000}) for (const std::int16_t b : {0x2600, 0x2650, 0x1000}) {
        Story s = base;
        s.contract_a = a; s.contract_8 = b; s.contract_c = a;
        s.table = {1, 2, 3, 4};
        build(s, club, regs(person_at, club, 0x34, 0), "caps");
    }
    // Other clubs of the owner: the draw over the club count (zero, small, beyond 16 bits).
    for (const std::uint32_t count : {0u, 1u, 5u, 40u, 70000u}) for (const auto& entries : std::vector<std::vector<std::uint32_t>>{{1, 2, 3, 4, 5, 6}, {1, 1, 1, 1, 1, 1}, {65534, 3, 4, 2, 3, 4}})
        for (const std::uint32_t row_owner : {0u, 1u, 2u}) {
            Story s = base;
            s.club_count = count; s.table = entries;
            build(s, club, regs(person_at, club, 0x34, 0), "other clubs");
            // The club table is redirected to the scratch rows so the wide draw's indexes stay mapped.
            append(cases.back().memory, {w32(scenario.toc(-0x7be8), clubs_at)});
            for (unsigned i = 0; i < 8; ++i) append(cases.back().memory, {w32(clubs_at + i * 0x248 + 0x2c, row_owner == 0 ? 0 : row_owner == 1 ? scenario.r32(club + 0x2c) : 0x5555),
                                                                           w32(clubs_at + i * 0x248 + 0x80, 0x66)});
        }

    // The tree: seeded random states. Values come from lists around the thresholds.
    std::mt19937 random(0x9e3779b9);
    const auto choose = [&](auto list) { return list[random() % list.size()]; };
    const std::vector<std::uint32_t> samples{0, 1, 2, 3, 4, 5, 6, 7, 11, 19, 20, 99, 149, 150, 1999, 2000, 65535, 65534, 40000};
    for (unsigned n = 0; n < 4000; ++n) {
        Story s = base;
        s.age = choose(std::vector<std::int16_t>{5, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 40, -1});
        s.byte_5e = choose(std::vector<std::uint8_t>{0, 0, 0, 0, 1, 12});
        s.grade = choose(std::vector<std::uint8_t>{0, 1, 2, 3, 5, 8, 20, 40, 0x7f, 0x80, 0xf0, 0xff});
        s.byte_67 = choose(std::vector<std::uint8_t>{0, 5, 0xf, 0x10, 0x20});
        s.byte_2f = choose(std::vector<std::uint8_t>{0x0, 0xf, 0x10, 0x20});
        s.word_20 = choose(std::vector<std::uint32_t>{0, 0, 1});
        s.contract_a = choose(std::vector<std::int16_t>{0x1000, 0x1700, 0x1c52, 0x1c53, 0x1f40, 0x1f41, 0x2000, 0x2328, 0x2329, 0x2800, 0x7fff, 0x0e00});
        s.contract_8 = choose(std::vector<std::int16_t>{0x500, 0x109a, 0x109b, 0x1800, 0x2000, 0x3000});
        s.contract_c = choose(std::vector<std::int16_t>{0x500, 0x800, 0x1b57, 0x1b58, 0x2134, 0x2135, 0x2800});
        s.contract_4 = choose(std::vector<std::int16_t>{0x100, 0x9a, 0xa0, 0xaf, 0xc4, 0xc5, 0x400});
        s.owner = choose(std::vector<std::int16_t>{0x100, 0x400, 0x800, 0x1000, 0x1500, 0x2000, 0x2800});
        s.club_rating = choose(std::vector<std::int16_t>{0x100, 0x800, 0x1000, 0x1800, 0x1964, 0x1d4c, 0x2000, 0x2500, 0x3000, 0x4000});
        s.date_day = choose(std::vector<std::int16_t>{1, 100, 365, 366, 200, 20, 0x7fff, -3});
        s.date_year = choose(std::vector<std::int16_t>{2000, 2001, 2002, 2003, 2001});
        s.today_day = choose(std::vector<std::int16_t>{1, 50, 100, 300, 365});
        s.today_year = choose(std::vector<std::int16_t>{2000, 2001, 2002});
        s.flags = choose(std::vector<std::uint32_t>{0, 0, 1, 8, 0x2000, 0x2009, 1});
        s.is_special = choose(std::vector<std::uint32_t>{1, 1, 1, 0});
        s.busy_answer = choose(std::vector<std::uint32_t>{0, 0, 1});
        s.league_answer = choose(std::vector<std::uint32_t>{0, 1, 2, 3});
        s.distance_answer = choose(std::vector<std::uint32_t>{0x100, 0x190, 0x191, 0x1000});
        s.staff_answer = choose(std::vector<std::uint32_t>{0, 1, 1});
        s.found_answer = choose(std::vector<std::uint32_t>{0, 1});
        s.open_answer = choose(std::vector<std::uint32_t>{0, 1});
        s.people_count = choose(std::vector<std::uint32_t>{0, 1, 4});
        s.coach_answer = choose(std::vector<std::uint32_t>{0, 1, 1, 1});
        s.club_count = choose(std::vector<std::uint32_t>{0, 3, 40});
        s.table.clear();
        for (unsigned i = 0; i < 16; ++i) s.table.push_back(choose(samples));
        s.seed = choose(std::vector<std::uint32_t>{0, 0, 0, 3, 0x1fff, 0x80000001u, 0xfffffff0u});
        s.wrap = choose(std::vector<int>{-1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
        const auto kind = choose(std::vector<std::uint32_t>{0x20, 0x20, 0x20, 0x33, 0x34, 0x5, 0x1ffu});
        const auto extra = choose(std::vector<std::uint32_t>{0u, extra_at});
        build(s, clubs[random() % clubs.size()], regs(person_at, club, kind, extra), "random");
    }
    // Targeted: the date branch (draw 1 % 3 != 0, today before the person's date + offset).
    for (const std::int16_t day : {1, 100, 300, 365}) for (const std::int16_t year : {2000, 2001, 2002, 2005}) {
        Story s = base;
        s.date_day = day; s.date_year = year; s.today_day = 100; s.today_year = 2001;
        s.table = {1, 1, 1, 1, 1, 1, 1, 1};
        build(s, club, regs(person_at, club, 0x20, 0), "date");
    }
    // The young player's first story (0x2e) with the table wrapping at each draw around it.
    for (int wrap = 0; wrap < 8; ++wrap) for (const std::uint32_t busy_value : {0u, 1u}) {
        Story s = base;
        s.age = 0x14; s.contract_a = 0x2000; s.club_rating = 0x1000; s.grade = 5; s.busy_answer = busy_value;
        s.table = {1, 3, 4, 1, 5, 6, 7, 8};
        s.wrap = wrap;
        build(s, club, regs(person_at, club, 0x20, 0), "young, wrapping");
    }
    // Story 0x2f (a settled player far from home with a high rating): each condition on and off, and the roll draw.
    for (const std::int16_t age : {0x1e, 0x1f}) for (const std::uint8_t b2f : {0xf, 0x10}) for (const std::uint32_t dist : {0x190u, 0x191u})
        for (const std::int16_t rating : {0x1c52, 0x1c53}) for (const std::uint8_t b67 : {0xf, 0x10}) for (const std::uint32_t roll : {3u, 4u}) {
            Story s = base;
            s.age = age; s.byte_2f = b2f; s.distance_answer = dist; s.contract_a = rating; s.byte_67 = b67;
            s.table = {1, 3, roll, 5, 6, 7, 8};
            build(s, club, regs(person_at, club, 0x20, 0), "settled");
        }
    // The ratio branches: every threshold on both sides with fixed table choices.
    for (const std::int16_t a : {0x1000, 0x1800, 0x2000}) for (const std::int16_t cr : {0x1000, 0x1800, 0x1900, 0x2000}) for (const std::uint8_t g : {0, 3, 0x7f, 0x80, 0xf0}) {
        Story s = base;
        s.age = 0x14; s.contract_a = a; s.club_rating = cr; s.grade = g;
        s.table = {1, 2, 1, 7, 5, 3, 1, 2};
        build(s, club, regs(person_at, club, 0x20, 0), "young");
        s.table = {1, 0, 5, 3, 1, 4, 2, 6};
        s.flags = 1;
        build(s, club, regs(person_at, club, 0x20, 0), "young, flag");
    }
}

// ---- Closing the last uncovered outcomes (central coverage run): the history already holds the story (0x1069e700 finds a row,
// which only real state steers), and the staff pick (0x10647730) returning nobody or a person with a club. The cases keep
// every callee that is a media routine real (an answer for one is ignored when it is checked), so the state they need is
// laid out: a one-row history in the press object, and one real person row turned into the only candidate of the staff pick.
namespace {
constexpr std::uint32_t history_at = scratch_base + 0x30000, nation_at = scratch_base + 0x30400;

// The fixture of the first section's `build`, written out once more for this generator.
Writes closure_memory(const Scenario& scenario, const Story& st, std::uint32_t club,
                      const std::vector<std::uint32_t>& clubs, std::uint32_t frame, std::uint32_t real_people) {
    Writes m;
    append(m, {w32(person_at, st.person_id), w32(person_at + 0xc, owner_at), w32(person_at + 0x14, contract_at), w32(person_at + 0x20, st.word_20),
               w32(person_at + 0x24, st.word_24 ? links_at + 0x100 : 0), w32(person_at + 0x28, links_at), w32(person_at + 0x2c, links_at + 0x40),
               w32(person_at + 0x30, links_at + 0x80), h16(person_at + 0x4c, st.date_day), h16(person_at + 0x4e, st.date_year), b8(person_at + 0x50, 0),
               h16(person_at + 0x5c, st.age), b8(person_at + 0x5e, st.byte_5e), b8(person_at + 0x63, st.grade), b8(person_at + 0x67, st.byte_67),
               w32(links_at + 0x20, 0x11), w32(links_at + 0x60, 0x22), w32(links_at + 0xa0, 0x33), w32(links_at + 0x100, 0x44), w32(links_at + 0x120, 0x55),
               h16(contract_at + 4, st.contract_4), h16(contract_at + 8, st.contract_8), h16(contract_at + 0xa, st.contract_a), h16(contract_at + 0xc, st.contract_c),
               b8(contract_at + 0xf, st.region), b8(contract_at + 0x2f, st.byte_2f),
               h16(owner_at + 0xac, st.owner), w32(owner_at + 0x68, 0x77),
               w32(standing_at + 0x50, st.flags), w32(standing_at + 0x44, 0),
               w32(extra_at, 0x1234), w32(extra_at + 0x1c, st.extra_pair & 1 ? club : clubs[1]), w32(extra_at + 0x20, st.extra_pair & 2 ? club : 0),
               b8(extra_at + 0x49, st.extra_pair >> 24), b8(extra_at + 0x4a, (st.extra_pair >> 16) & 0xff),
               w32(staff_at, 0x4321), w32(staff_at + 0x20, st.staff_20 ? links_at + 0x100 : 0), w32(staff_at + 0x24, st.staff_24 ? links_at + 0x120 : 0),
               w32(staff_at + 0x28, links_at), w32(staff_at + 0x2c, links_at + 0x40), w32(staff_at + 0x30, links_at + 0x80),
               b8(staff_at + 0x60, st.staff_60), b8(staff_at + 0x62, st.staff_62),
               h16(found_at, 0x1234), h16(found_at + 2, 0x4567), w32(found_at + 8, 0x88),
               w32(chain_at, chain_at + 0x40), w32(chain_at + 0x48, 0x99), w32(word_at, 0xabc),
               w32(frame + 0x44, found_at),
               w32(club + 0x30, st.special_ptr ? st.special_ptr : special_at), w32(special_at, 5), w32(scenario.toc(-0x7a0c), st.same_id ? 5 : 6),
               w32(scenario.toc(-0x7b50), st.club_count)});
    for (unsigned i = 0; i < 30; ++i) m.push_back(w32(frame + 0x7c + 4 * i, real_people + (i % 7 + 1) * 0x70));
    const auto clock = scenario.toc(-0x7b80);
    append(m, {h16(clock, st.today_day), h16(clock + 2, st.today_year), w32(clock + 4, 0)});
    m.push_back(h16(club + 0x1ec, st.club_rating));
    append(m, random_table(scenario, st.table, st.seed, st.wrap));
    return m;
}
}

MEDIA_CASES("pick_b_bdd_closure") {
    std::vector<std::uint32_t> clubs;
    for (unsigned i = 0; clubs.size() < 3 && i < 400; ++i) {
        const auto c = scenario.club(i);
        const auto manager = scenario.r32(c + 0x90);
        if (manager && scenario.r32(c + 0x2c) && scenario.r32(c + 0x30) && scenario.r32(manager + 0x28) && scenario.r32(manager + 0x2c) && scenario.r32(manager + 0x30))
            clubs.push_back(c);
    }
    if (clubs.size() < 3) return;
    const auto sp = std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(1)));
    const auto frame = sp - 0x240;
    const auto press = scenario.global(-0x7b94);
    const auto real_people = scenario.global(-0x7be0);
    const auto club = clubs[0];
    const auto push = [&](const Story& st, std::uint32_t kind, std::uint32_t extra, Writes more, const char* label) {
        auto memory = closure_memory(scenario, st, club, clubs, frame, real_people);
        append(memory, more);
        Answers answers{{special_club, st.is_special}, {standing_of, standing_at}, {busy, st.busy_answer}, {league, st.league_answer}, {distance, st.distance_answer},
                        {known, st.known_answer}, {staff_pick, st.staff_answer ? staff_at : 0}, {seller, st.seller_answer}, {found_of, st.found_answer}, {chain_of, chain_at},
                        {people_of, st.people_count}, {open_club, st.open_answer}, {coach_of, st.coach_answer ? word_at : 0}, {form, st.form_answer},
                        {roll_of, 3}, {sale, st.sale_answer}, {follow_up, 0}, {owner_hook, 0}};
        cases.push_back({.entry = entry_bdd, .registers = {{3, press}, {4, person_at}, {5, club}, {6, kind}, {7, extra}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}},
                         .memory = std::move(memory), .f1 = {}, .label = label, .answers = std::move(answers)});
    };
    Story base;
    base.table = {1, 2, 3, 4, 5, 6};

    // The history already holds the story (0x1069e700 finds the row of this person, club, story 0xbdd and kind): the routine ends
    // before the numbered fields. A row of another kind, club or person does not end it.
    const auto club_id = scenario.r32(club);
    const auto history = [&](std::uint32_t person, std::uint32_t row_club, std::uint32_t grade, std::uint32_t story) {
        return Writes{w32(press, history_at), w32(press + 4, 8), w32(press + 8, 1), w32(history_at + 8, person), w32(history_at + 0xc, row_club),
                      b8(history_at + 0x10, 0), h16(history_at + 0x12, grade), w32(history_at + 0x14, story)};
    };
    for (const std::uint32_t kind : {0x33u, 0x34u}) {
        push(base, kind, 0, history(base.person_id, club_id, kind, 0xbdd), "history holds the story");
        push(base, kind, extra_at, history(base.person_id, club_id, kind, 0xbdd), "history holds the story, extra");
        push(base, kind, 0, history(base.person_id, club_id, kind + 1, 0xbdd), "history, other kind");
        push(base, kind, 0, history(base.person_id, club_id ^ 1, kind, 0xbdd), "history, other club");
        push(base, kind, 0, history(base.person_id + 1, club_id, kind, 0xbdd), "history, other person");
        push(base, kind, 0, history(base.person_id, club_id, kind, 0xbde), "history, other story");
    }

    // The staff pick (0x10647730 with the person as `other`) over a table of one person, the first record of the table, turned into
    // the only candidate: old enough (age byte 0x30, ability 60), a staff record, not a player, not the club's manager.
    const auto staff = [&](std::uint32_t nation, std::uint32_t at_club, const char* label, std::uint32_t kind) {
        Writes more{w32(scenario.toc(-0x7be4), 1), h16(real_people + 0x5c, 0x30), b8(real_people + 0x5e, 60), w32(real_people + 0x14, 0),
                    w32(real_people + 0x1c, staff_at), w32(real_people + 0x24, at_club), w32(real_people + 0xc, nation), w32(real_people + 0x20, 0),
                    b8(real_people + 0x60, 7), b8(real_people + 0x62, 5), w32(real_people + 0x28, links_at), w32(real_people + 0x2c, links_at + 0x40),
                    w32(real_people + 0x30, links_at + 0x80), h16(nation_at + 0xac, 0), w32(nation_at + 0x68, 0x66)};
        for (const std::uint32_t extra : {0u, extra_at}) push(base, kind, extra, more, label);
    };
    // A candidate of another nation than the person's, with a rating of 0, fails the stranger's rating test on every round: nobody.
    for (const std::uint32_t kind : {0x33u, 0x34u, 0x20u}) staff(nation_at, 0, "staff pick finds nobody", kind);
    // A candidate of the person's own nation (his score doubles) who works for a club: the staff record of a coach.
    for (const std::uint32_t kind : {0x33u, 0x34u, 0x20u}) {
        staff(owner_at, club, "staff pick finds a coach with a club", kind);
        staff(owner_at, 0, "staff pick finds a coach without a club", kind);
    }
}
