// The values block's cases: the random draw (every path and sign), then the routines that
// draw in line and build the press's records.
#include "media_checks.hpp"
#include "guest_abi.hpp"
#include <limits>
#include <string_view>

using namespace cm::aot::media_checks;

namespace {
constexpr std::uint32_t table_at = scratch_base + 0x8000;
// The random table as the game keeps it: the cursor steps through uint16 entries at `table_at`
// (the first draw reads entries[0]), the seed is added to each. `refill` puts the cursor at the
// end so the next step wraps to the start and draws a new seed with rand().
std::vector<Write> table(const Scenario& s, const std::vector<std::uint32_t>& entries, std::uint32_t seed, bool refill = false, unsigned skip = 0) {
    std::vector<Write> writes;
    const auto end = table_at + 0x100;
    for (std::size_t i = 0; i < entries.size(); ++i) writes.push_back({2, table_at + 2 * std::uint32_t(i + 1), entries[i]});
    // With `skip` steps before the wrap, those steps read the entries just below the end.
    if (refill) for (unsigned j = 1; j <= skip && j <= entries.size(); ++j) writes.push_back({2, end - 2 * skip + 2 * j, entries[j - 1]});
    writes.push_back({4, s.toc(-0x7b84), refill ? end - 2 * skip : table_at});
    writes.push_back({4, s.toc(-0x7b88), end});
    writes.push_back({4, s.toc(-0x7b8c), table_at});
    writes.push_back({4, s.toc(-0x7b90), seed});
    if (refill) for (std::uint32_t i = 1; i <= 4; ++i) writes.push_back({2, table_at + 2 * i, 0x1111u * i});
    return writes;
}
std::vector<std::pair<unsigned, std::uint32_t>> scratch_registers(std::uint32_t first) {
    return {{3, first}, {4, 0x41414141}, {5, 0x51515151}, {6, 0x61616161}, {7, 0x71717171}, {8, 0x81818181}, {9, 0x91919191}, {10, 0xa1a1a1a1}};
}
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
Write b8(std::uint32_t at, std::uint32_t v) { return {1, at, v}; }
Write h16(std::uint32_t at, std::uint32_t v) { return {2, at, v}; }
Write w32(std::uint32_t at, std::uint32_t v) { return {4, at, v}; }
// Real persons (the 0x70-byte table at -0x7be0) that have a contract (+0x14), and real clubs that have a manager (+0x90).
std::vector<Write> joined_writes(std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; }
std::vector<std::uint32_t> contracted_persons(const Scenario& s, unsigned count) {
    std::vector<std::uint32_t> found;
    const auto base = s.global(-0x7be0);
    for (unsigned i = 0; found.size() < count && i < 5000; ++i)
        if (s.r32(base + i * 0x70 + 0x14)) found.push_back(base + i * 0x70);
    return found;
}
std::vector<std::uint32_t> managed_clubs(const Scenario& s, unsigned count) {
    std::vector<std::uint32_t> found;
    for (unsigned i = 0; found.size() < count && i < 400; ++i)
        if (s.r32(s.club(i) + 0x90)) found.push_back(s.club(i));
    return found;
}
}

MEDIA_CASES("values") {
    // 0x10647c00 random_below: zero, within 16 bits (both signs), the float form with the finer draw
    // (positive, clamped, negative, a zero finer range) and the table wrapping.
    const std::uint32_t ranges[] = {0, 1, 5, 0xffffffffu, 0xfffffffbu, 100, 0xffff, 0xffff0001u, 0x10000, 0xffff0000u, 0x10001, 100000, 0xfffe7960u,
                                    1000000, 3000000, 0xffd23940u, 214748364, 214748365, 429496729, 0x19999999, 0x7fffffff, 0x80000000u, 0x80000001u};
    const std::vector<std::vector<std::uint32_t>> samples{{0, 0}, {65534, 1}, {65534, 65534}, {1, 65534}, {1000, 5}, {65535, 65535}};
    for (const auto n : ranges)
        for (const auto& entries : samples)
            cases.push_back({.entry = 0x10647c00, .registers = scratch_registers(n), .memory = table(scenario, entries, 0), .f1 = {}, .label = {}});
    for (const auto n : {7u, 0xfffffff9u, 100000u, 0xfffe7960u, 214748364u, 429496729u, 0x7fffffffu})
        cases.push_back({.entry = 0x10647c00, .registers = scratch_registers(n), .memory = table(scenario, {7, 9}, 0x1fff, true), .f1 = {}, .label = "wraps"});
    for (const auto n : {100000u, 3000000u, 214748364u})
        cases.push_back({.entry = 0x10647c00, .registers = scratch_registers(n), .memory = table(scenario, {40000, 50000}, 60000), .f1 = {}, .label = "seeded"});
}

// Distinctive leftovers in r3-r10 under the arguments, so what a routine leaves of them shows.
Registers args(std::initializer_list<std::pair<unsigned, std::uint32_t>> set) {
    Registers r = scratch_registers(0x31313131);
    for (const auto& [n, v] : set) r[n - 3] = {n, v};
    return r;
}

// 0x10649900 post_item_bdb(person, club, kind): the guards, then every kind of the decision tree with
// each draw remainder (including a negative sample) and the table wrapping.
MEDIA_CASES("values-press") {
    const auto people = contracted_persons(scenario, 6);
    const auto clubs = managed_clubs(scenario, 3);
    const auto person = people[0], club = clubs[0];
    const auto bare = scenario.global(-0x7be0) + 3 * 0x70;  // no contract
    const auto call = [&](std::uint32_t entry, Registers registers, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, const char* label = "") {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = label, .answers = std::move(answers)});
    };
    // 0x10649900
    call(0x10649900, args({{4, 0}, {5, club}, {6, 3}}), {}, {}, "no person");
    call(0x10649900, args({{4, bare}, {5, club}, {6, 3}}), {}, {}, "no contract");
    call(0x10649900, args({{4, person}, {5, 0}, {6, 3}}), {}, {}, "no club");
    for (const std::int32_t kind : {-5, -1, 0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 34, 35, 36, 0x7fff, 0x8000, 0x10003})
        for (const auto& samples : std::vector<std::pair<std::vector<std::uint32_t>, std::uint32_t>>{{{0}, 0}, {{1}, 0}, {{2}, 0}, {{3}, 0}, {{4}, 0}, {{7}, 5}, {{1}, 0x80000001u}, {{2}, 0xfffffff0u}})
            for (const auto who : {people[0], people[1]})
                call(0x10649900, args({{4, who}, {5, clubs[kind & 1]}, {6, std::uint32_t(kind)}}), table(scenario, samples.first, samples.second));
    for (const std::int32_t kind : {0, 2, 17, 35, 1})
        call(0x10649900, args({{4, person}, {5, club}, {6, std::uint32_t(kind)}}), table(scenario, {7}, 0x1fff, true), {}, "wraps");

    // 0x1067d040 post_item_bcf(person, club, kind, extra): the guards, both rating tiers, each rank and
    // the distance test, with 0x1014c2e0, 0x100d3150 and 0x10718130 answered.
    const auto contract = [&](std::uint32_t p) { return scenario.r32(p + 0x14); };
    call(0x1067d040, args({{4, 0}, {5, club}, {6, 0x20}, {7, 1}}), {}, {}, "no person");
    call(0x1067d040, args({{4, bare}, {5, club}, {6, 0x20}, {7, 1}}), {}, {}, "no contract");
    call(0x1067d040, args({{4, person}, {5, 0}, {6, 0x20}, {7, 1}}), {}, {}, "no club");
    call(0x1067d040, args({{4, person}, {5, club}, {6, 0x20}, {7, 1}}), {}, {{0x1014c2e0, 0}}, "club refused");
    for (const auto who : people) call(0x1067d040, args({{4, who}, {5, club}, {6, 0x20}, {7, 1}}), {}, {}, "real");
    for (const std::uint32_t rating : {5000u, 4000u})
        for (const std::uint32_t b63 : {16u, 5u})
            for (const std::uint32_t b69 : {3u, 8u})
                for (const std::uint32_t b65 : {16u, 3u, 7u})
                    for (const std::uint32_t b67 : {11u, 5u, 12u})
                        for (const std::uint32_t kind : {0x20u, 7u})
                            for (const std::uint32_t distance : {0x100u, 0xd2u}) {
                                const auto memory = std::vector<Write>{h16(contract(person) + 0xa, rating), b8(person + 0x63, b63), b8(person + 0x69, b69),
                                                                        b8(person + 0x65, b65), b8(person + 0x67, b67)};
                                call(0x1067d040, args({{4, person}, {5, club}, {6, kind}, {7, 0x12345}}), memory, {{0x1014c2e0, 1}, {0x100d3150, b63 & 1}, {0x10718130, distance}});
                            }
    for (const std::uint32_t kind : {1u, 0x1eu, 0x20u, 7u, 0xffu, 0x180u})
        for (const std::uint32_t rating : {5000u, 4000u})
            for (const std::uint32_t accepted : {0u, 1u, 0x100u})
                for (const std::uint32_t extra : {0u, 5u, 0xffff8000u, 0x18000u}) {
                    const auto memory = std::vector<Write>{h16(contract(person) + 0xa, rating), b8(person + 0x63, 5), b8(person + 0x65, 16), b8(person + 0x67, 12)};
                    call(0x1067d040, args({{4, person}, {5, club}, {6, kind}, {7, extra}}), memory, {{0x1014c2e0, 1}, {0x100d3150, accepted}, {0x10718130, 0x1000}});
                }

    // 0x1067d830 post_item_bdc(person, club, figure, grade): the guards, the young-player flag, the tier
    // ladder (0 to 4) over the standing answers, the club rating bit trick and the posts.
    const std::uint32_t record = scratch_base + 0x100;
    const auto rating_row = [&](std::uint32_t k) { return scenario.r32(scenario.global(-0x79f0)) + scenario.r32(k) * 0x4c + 6; };
    call(0x1067d830, args({{4, 0}, {5, club}, {6, 9}, {7, 2}}), {}, {}, "no person");
    call(0x1067d830, args({{4, bare}, {5, club}, {6, 9}, {7, 2}}), {}, {}, "no contract");
    call(0x1067d830, args({{4, person}, {5, 0}, {6, 9}, {7, 2}}), {}, {}, "no club");
    call(0x1067d830, args({{4, person}, {5, clubs[2]}, {6, 9}, {7, 2}}), {w32(clubs[2] + 0x90, 0)}, {}, "no manager");
    call(0x1067d830, args({{4, person}, {5, club}, {6, 9}, {7, 2}}), {}, {{0x10082f90, 0}}, "no standing record");
    for (const auto who : people)
        for (const auto c : clubs) call(0x1067d830, args({{4, who}, {5, c}, {6, 9}, {7, 2}}), {}, {}, "real");
    for (const std::uint32_t flags : {0u, 8u, 0x2000u, 0x181u, 0x2181u, 0x8u | 0x100u})
        for (const std::uint32_t standing : {0u, 1u, 2u})
            for (const std::uint32_t rating : {5000u, 4000u})
                for (const std::uint32_t club_rating : {100u, 6000u})
                    for (const std::uint32_t shape : {0u, 1u, 2u, 3u})
                        for (const std::uint32_t grade : {2u, 3u})
                            for (const std::uint32_t distance : {0x400u, 0x3e8u}) {
                                // shape: which of the player's bytes pass (young: 0x67 < 10, 0x69 > 10, 0x65 < 15, 0x5c > 23)
                                const bool a = shape & 1, b = shape & 2;
                                const auto memory = std::vector<Write>{w32(record + 0x50, flags), h16(contract(person) + 0xa, rating), h16(club + 0x1ec, club_rating),
                                    b8(person + 0x67, a ? 5 : 12), b8(person + 0x69, a ? 12 : 3), b8(person + 0x65, a ? 5 : 18), b8(person + 0x63, b ? 16 : 4),
                                    h16(person + 0x5c, a ? 0x30 : 0x10)};
                                call(0x1067d830, args({{4, person}, {5, club}, {6, 0x1234}, {7, grade}}), memory,
                                     {{0x10082f90, record}, {0x1007c140, standing}, {0x10718130, distance}});
                            }
    for (const std::uint32_t value : {0x0u, 0xfa0u, 0xf9fu, 0xfa1u, 0xffffu, 0x7fffu, 0x8000u, 0x8001u, 0x1u})
        call(0x1067d830, args({{4, person}, {5, club}, {6, 77}, {7, 2}}), {h16(rating_row(club), value)}, {{0x10082f90, record}}, "club rating");
    for (const auto c : clubs)
        call(0x1067d830, args({{4, person}, {5, c}, {6, 77}, {7, 2}}), {w32(scenario.toc(-0x7b40), 0)}, {{0x10082f90, record}}, "nobody listed");
    // The post loop with a known listing size (0x103fc710 answered) and the post's own result answered: the
    // original returns r3 = the loop test's sign-extended count, whatever the last post said.
    for (const std::uint32_t listed : {0u, 1u, 2u, 3u, 5u, 0x7fu, 0x80u, 0xffu, 0x101u})
        for (const std::uint32_t posted : {0u, 1u})
            call(0x1067d830, args({{4, person}, {5, club}, {6, 77}, {7, 2}}), {},
                 {{0x10082f90, record}, {0x103fc710, listed}, {0x102cc140, posted}}, ("post loop listed " + std::to_string(listed) + " posted " + std::to_string(posted)).c_str());

    // 0x10647730 pick_staff(club, other): a one-person table to steer every test, plus the real table.
    {
        const auto first = scenario.global(-0x7be0);
        const std::uint32_t owner_a = scratch_base + 0x400, owner_b = scratch_base + 0x600, record = scratch_base + 0x800;
        call(0x10647730, args({{4, 0}, {5, 0}}), {}, {}, "nobody");
        call(0x10647730, args({{4, club}, {5, 0}}), {}, {}, "real table");
        call(0x10647730, args({{4, club}, {5, people[1]}}), {}, {}, "real table, other");
        for (const std::uint32_t count : {0u, 1u, 3u}) {
            for (const std::uint32_t fault : {0u, 1u, 2u, 3u, 4u, 5u})
                for (const std::uint32_t owner_choice : {0u, 1u, 2u})
                    for (const std::uint32_t rating : {7000u, 100u, 0u})
                        for (const std::uint32_t other_choice : {0u, 1u}) {
                            if (count != 1 && (fault || owner_choice == 1 || rating != 7000 || other_choice)) continue;
                            std::vector<Write> memory{w32(scenario.toc(-0x7be4), count), b8(first + 0x5e, fault == 2 ? 49 : 60), h16(first + 0x5c, fault == 1 ? 36 : 0x30),
                                w32(first + 0x1c, fault == 3 ? 0 : record), w32(first + 0x14, fault == 4 ? record : 0), w32(first + 0x24, fault == 4 ? record : 0),
                                w32(club + 0x90, fault == 5 ? first : 0), w32(first + 0xc, owner_choice == 2 ? owner_b : owner_a),
                                h16(owner_a + 0xac, rating), h16(owner_b + 0xac, 7000), w32(club + 0x2c, owner_choice == 0 ? owner_a : 0xfeed),
                                w32(people[1] + 0xc, owner_choice == 1 ? owner_a : owner_b)};
                            call(0x10647730, args({{4, club}, {5, other_choice ? people[1] : 0}}), memory, {{0x100d9580, owner_choice}}, "steered");
                        }
        }
        for (const std::uint32_t link_rating : {7000u, 100u})
            for (const std::uint32_t rating : {7000u, 6000u, 20u})
                for (const std::uint32_t verdict : {0u, 1u}) {
                    std::vector<Write> memory{w32(scenario.toc(-0x7be4), 2), b8(first + 0x5e, 60), h16(first + 0x5c, 0x30), w32(first + 0x1c, record), w32(first + 0x14, 0),
                        w32(club + 0x90, 0), w32(first + 0xc, owner_a), h16(owner_a + 0xac, rating), h16(record + 0x2c, link_rating), w32(club + 0x2c, 0xfeed),
                        b8(first + 0x70 + 0x5e, 70), h16(first + 0x70 + 0x5c, 0x30), w32(first + 0x70 + 0x1c, record), w32(first + 0x70 + 0xc, owner_a),
                        w32(people[1] + 0xc, owner_b)};
                    call(0x10647730, args({{4, club}, {5, people[1]}}), memory, {{0x100d9580, verdict}}, "strangers");
                }
    }

    // 0x106548e0 post_item_bda(list, person, club, kind, extra): the guards, the kind gate and its draw
    // against +0x69, every kind of the switch with each remainder, and the morale byte.
    {
        const std::uint32_t standing = scratch_base + 0x180, list = scratch_base + 0x1c0;
        call(0x106548e0, args({{3, list}, {4, 0}, {5, club}, {6, 0}, {7, 1}}), {}, {}, "no person");
        call(0x106548e0, args({{3, list}, {4, bare}, {5, club}, {6, 0}, {7, 1}}), {}, {}, "no contract");
        call(0x106548e0, args({{3, list}, {4, person}, {5, 0}, {6, 0}, {7, 1}}), {}, {}, "no club");
        call(0x106548e0, args({{3, list}, {4, person}, {5, clubs[2]}, {6, 0}, {7, 1}}), {w32(clubs[2] + 0x90, 0)}, {}, "no manager");
        call(0x106548e0, args({{3, list}, {4, person}, {5, club}, {6, 0}, {7, 1}}), {}, {{0x10082f90, 0}}, "no standing");
        for (const auto who : people) call(0x106548e0, args({{3, list}, {4, who}, {5, club}, {6, 2}, {7, 1}}), {w32(list, 0), w32(list + 8, 0)}, {}, "real");
        // The list 0x10658370 reads for the gate: one row naming the person, with `mark` at +0x10 (2 is the one that counts).
        const std::uint32_t rows = scratch_base + 0x2000;
        const auto known_row = [&](std::uint32_t mark) {
            return std::vector<Write>{w32(list, rows), w32(list + 8, 1), w32(rows + 8, scenario.r32(person)), b8(rows + 0x10, mark)};
        };
        std::vector<std::int32_t> kinds{-1, 0x1000, 0x82, 0x8000};
        for (std::int32_t k = 0; k <= 0x26; ++k) kinds.push_back(k);
        const std::vector<std::pair<std::vector<std::uint32_t>, std::uint32_t>> draws{{{0, 0}, 0}, {{1, 4}, 0}, {{2, 3}, 0}, {{3, 2}, 0}, {{4, 1}, 0}, {{9, 7}, 0}, {{5, 6}, 0}, {{1, 1}, 0x80000001u}, {{2, 2}, 0xfffffff0u}};
        // The gate's list itself: a row of another kind, a row of another person, two rows (the second counts), none.
        for (const auto& [label, writes] : std::vector<std::pair<const char*, std::vector<Write>>>{
                 {"gate list: row of kind 1", known_row(1)},
                 {"gate list: row of another person", joined_writes(known_row(2), {w32(rows + 8, scenario.r32(person) + 1)})},
                 {"gate list: second row counts", joined_writes(known_row(2), {w32(list + 8, 2), w32(rows + 8, scenario.r32(person) + 1), w32(rows + 0x18 + 8, scenario.r32(person)), b8(rows + 0x18 + 0x10, 2)})},
                 {"gate list: empty", known_row(2)}})
            for (const std::uint32_t kind : {0u, 2u, 0xdu, 0x23u}) {
                auto memory = joined_writes(table(scenario, {1, 4}, 0), writes);
                memory = joined_writes(memory, {b8(person + 0x69, 5)});
                if (std::string_view(label) == "gate list: empty") memory = joined_writes(memory, {w32(list + 8, 0)});
                call(0x106548e0, args({{3, list}, {4, person}, {5, club}, {6, kind}, {7, 0x1ff}}), memory, {{0x10082f90, standing}, {0x1018c340, 0}}, label);
            }
        for (const auto kind : kinds)
            for (const auto& [entries, seed] : draws) {
                // The kind gate: the list knows the person (answer 1), or the standing carries flag 8 or 0x2000.
                for (const std::uint32_t gate : {0u, 1u, 2u}) {
                    // gate 0 is a list that does know the person (a row of kind 2), the others an empty list.
                    const std::vector<Write> known = gate == 0 ? known_row(2) : std::vector<Write>{w32(list, 0), w32(list + 8, 0)};
                    const std::vector<Write> memory = joined_writes(joined_writes(table(scenario, entries, seed), known),
                                                                    {w32(standing + 0x50, gate == 2 ? 8 : gate == 1 ? 0x2000 : 0), b8(person + 0x69, 5)});
                    call(0x106548e0, args({{3, list}, {4, person}, {5, club}, {6, std::uint32_t(kind)}, {7, 0x1ff}}), memory,
                         {{0x10082f90, standing}, {0x1018c340, 0}}, "kinds");
                }
            }
        for (const std::uint32_t spoken : {1u, 0u})
            for (const std::uint32_t byte69 : {0u, 5u, 9u, 0x80u})
                for (const auto& [entries, seed] : draws) {
                    const std::vector<Write> memory = joined_writes(table(scenario, entries, seed), {w32(standing + 0x50, 8), b8(person + 0x69, byte69)});
                    call(0x106548e0, args({{3, list}, {4, person}, {5, club}, {6, 4}, {7, 3}}), joined_writes(memory, {w32(list, 0), w32(list + 8, 0)}), {{0x10082f90, standing}, {0x1018c340, spoken}}, "gate draw");
                }
        for (const std::uint32_t morale : {0u, 9u, 10u, 0x13u, 0x14u, 0x80u})
            for (const std::int32_t kind : {0, 6, 0xe, 0x11})
                for (const auto& [entries, seed] : draws)
                    for (const std::uint32_t deputy : {0u, 1u}) {
                        const std::vector<Write> memory = joined_writes(table(scenario, entries, seed), {w32(standing + 0x50, 0), b8(contract(person) + 0x44, morale), w32(club + 0x94, deputy ? people[1] : 0)});
                        call(0x106548e0, args({{3, list}, {4, person}, {5, club}, {6, std::uint32_t(kind)}, {7, 0}}), joined_writes(memory, {w32(list, 0), w32(list + 8, 0)}), {{0x10082f90, standing}, {0x1018c340, 0}}, "morale");
                    }
    }

    // 0x10661a50 story_value(f1 scale, person, club, standing, kind, grade): every kind of the switch,
    // each state byte, scales that push the figure below and above the cutoff, both clamps of the ratings,
    // and the missing standing.
    {
        const std::uint32_t standing = scratch_base + 0x240;
        const auto story = [&](double scale, std::uint32_t who, std::uint32_t at, std::uint32_t record, std::uint32_t kind, std::uint32_t grade, std::vector<Write> memory, const char* label) {
            cases.push_back({.entry = 0x10661a50, .registers = args({{4, who}, {5, at}, {6, record}, {7, kind}, {8, grade}}), .memory = std::move(memory), .f1 = scale, .label = label});
        };
        story(1.0, person, club, 0, 0, 3, {w32(scenario.toc(-0x7b7c), scenario.r32(scenario.toc(-0x7b7c)))}, "real standing");
        for (const auto who : people) story(2.5, who, club, 0, 0x11, 3, {}, "real standing");
        std::vector<std::uint32_t> kinds{0xffff8000u, 0x1000, 0x53, 0x52, 0x82};
        for (std::uint32_t k = 0; k <= 0x26; ++k) kinds.push_back(k);
        for (const auto kind : kinds)
            for (const double scale : {0.25, 300000.0, -300000.0, 1e12, -1e12})
                for (const std::uint32_t state : {0u, 1u, 2u, 3u, 4u, 7u, 8u})
                    for (const std::uint32_t grade : {3u, 5u}) {
                        if ((state != 0 && kind != 0 && kind != 3 && kind != 0x11 && kind != 0x23) || (grade != 5 && kind != 0x23)) continue;
                        story(scale, people[kind & 1], clubs[(kind >> 1) & 1], standing, kind, grade, {b8(standing + 0x60, state | 0x30)}, "kinds");
                    }
        for (const std::uint32_t club_rating : {100u, 2000u, 5000u, 7500u, 9000u, 0xffff8000u})
            for (const std::uint32_t contract_rating : {100u, 2000u, 3000u, 9999u})
                for (const std::uint32_t kind : {1u, 0x11u, 4u}) {
                    const auto memory = std::vector<Write>{h16(club + 0x1ec, club_rating), h16(contract(person) + 0xa, contract_rating), b8(standing + 0x60, 1)};
                    story(-300000.0, person, club, standing, kind, 5, memory, "ratings");
                    story(300000.0, person, club, standing, kind, 5, memory, "ratings");
                }
        for (const double scale : {0.0, -0.0, 1e300, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            for (const std::uint32_t kind : {0u, 1u, 0x1eu, 0x52u})
                story(scale, person, club, standing, kind, 5, {b8(standing + 0x60, 2)}, "special scales");
    }

    // 0x1066b600 and 0x1066bc50 change_rating_a/b(club, person, kind, change, grade[, flag]): the club's rating row
    // at the two columns, the change through both queries and both limits, every classification, the report's
    // optional parts and the flags about the person.
    {
        const auto ratings = scenario.r32(scenario.global(-0x79f0));
        const auto row = [&](std::uint32_t at) { return ratings + scenario.r32(at) * 0x4c; };
        const std::uint32_t standing = scratch_base + 0x2c0;
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> queries{{1, 0}, {0, 1}, {0, 0}};
        const std::vector<std::uint32_t> changes{0, 100, 0xffffff9cu, 400, 600, 0xfffffda8u, 900, 1100, 0xfffffbb4u, 5000, 0x7ffff, 0x18000, 0x8000, 0xfffff000u};
        for (const bool second : {false, true}) {
            const std::uint32_t entry = second ? 0x1066bc50 : 0x1066b600, column = second ? 6 : 8;
            const std::uint32_t first_query = second ? 0x100d9520 : 0x100d9480, second_query = second ? 0x100d9580 : 0x100d94d0;
            const auto run = [&](std::uint32_t at, std::uint32_t who, std::uint32_t kind, std::uint32_t change, std::uint32_t grade, std::vector<Write> memory,
                                 std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, const char* label) {
                call(entry, args({{4, at}, {5, who}, {6, kind}, {7, change}, {8, grade}, {9, 1}}), std::move(memory), std::move(answers), label);
            };
            for (const std::uint32_t rating : {100u, 5000u, 6000u, 6251u, 4499u, 9000u, 20000u, 0x7fffu, 0x8000u, 1u, 0x2710u})
                for (const auto change : changes)
                    for (const auto& [a, b] : queries) {
                        const std::vector<Write> memory{h16(row(club) + column, rating)};
                        run(club, people[1], 0x33, change, 4, memory, {{first_query, a}, {second_query, b}, {0x10082f20, 0}, {0x10718130, 3000}, {0x10084be0, 0x500000}}, "rating");
                    }
            for (const auto at : clubs) run(at, 0, 7, 700, 1, {}, {{first_query, 0}, {second_query, 0}}, "no person");
            for (const auto who : people) run(clubs[0], who, 7, 700, 1, {}, {}, "real");
            run(0, people[0], 7, 100, 1, {}, {{first_query, 0}, {second_query, 0}}, "no club");
            // The report's draw, with and without a wrap.
            for (const std::uint32_t refill : {0u, 1u})
                for (const std::uint32_t draw : {0u, 1u, 2u, 3u, 5u})
                    run(club, people[1], 3, 1100, 4, table(scenario, {draw}, refill ? 0x80000001u : 0, refill), {{first_query, 0}, {second_query, 1}, {0x10082f20, 0}}, "draw");
            // The person's flags (1066bc50): standing state, ability bytes, distance and cost.
            for (const std::uint32_t state : {0u, 1u, 2u, 3u})
                for (const std::uint32_t ability : {5u, 0xfu, 0x15u})
                    for (const std::uint32_t grade : {0u, 1u, 100u, 0xffffff80u})
                        for (const std::uint32_t distance : {0x5dcu, 0x5ddu})
                            for (const std::uint32_t cost : {0x44aa20u, 0x44aa21u})
                                for (const std::uint32_t flag : {0u, 1u, 0x100u}) {
                                    const auto who = people[0];
                                    const std::vector<Write> memory{h16(row(club) + column, 7000), w32(standing + 0x60, state), b8(who + 0x65, ability), b8(who + 0x67, ability),
                                                                    h16(who + 0x5c, 0x30), h16(contract(who) + 4, ability * 10), h16(row(club) + 0xe, 2222)};
                                    call(entry, args({{4, club}, {5, who}, {6, 1}, {7, 900}, {8, 2}, {9, flag}}), memory,
                                         {{first_query, 0}, {second_query, 0}, {0x10082f20, standing}, {0x10303090, grade}, {0x10718130, distance}, {0x10084be0, cost}}, "flags");
                                }
            // The report without a rival record, and a person without a contract.
            run(club, people[1], 3, 900, 4, {w32(club + 0x80, 0)}, {{first_query, 0}, {second_query, 0}}, "no rival");
            run(club, bare, 3, 900, 4, {}, {{first_query, 1}, {second_query, 0}, {0x10082f20, standing}}, "no contract");
        }
    }

    // 0x106487d0 find_manager(club): the guards, the managers' list, each of the three staff members with every
    // byte draw and test, and the draw over the other clubs (narrow and beyond 16 bits).
    {
        const std::uint32_t sp = std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(1))), frame = sp - 0xe0;
        const std::uint32_t manager_record = scratch_base + 0x5000, link = 0x1234;
        const auto staff_at = [&](unsigned i) { return scratch_base + 0x3000 + i * 0x200; };
        const auto detail_at = [&](unsigned i) { return scratch_base + 0x4000 + i * 0x100; };
        const auto entry_at = [&](unsigned i) { return scratch_base + 0x6000 + i * 0x40; };
        const auto rating_at = [&](unsigned i) { return scratch_base + 0x6800 + i * 0x40; };
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> calm{{0x100db060, 0}, {0x100c7d00, 1}, {0x10464160, 0}};
        const auto base = [&](std::uint32_t at) {
            return std::vector<Write>{w32(at + 0x90, manager_record), w32(manager_record + 0x1c, 1), w32(at + 0x30, link), h16(at + 0x1ec, 1000),
                                      w32(at + 0x74, 0), w32(at + 0x78, 0), w32(at + 0x7c, 0), b8(frame + 0x38, 0)};
        };
        const auto plus = [&](std::vector<Write> a, const std::vector<Write>& b) { return joined_writes(std::move(a), b); };
        const auto find = [&](std::uint32_t at, std::vector<Write> memory, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers, const char* label) {
            call(0x106487d0, args({{4, at}}), std::move(memory), std::move(answers), label);
        };
        // The cost of the draw over clubs: nothing to draw (count 0) unless a case sets it.
        const std::vector<Write> no_clubs{w32(scenario.toc(-0x7b50), 0), w32(scenario.toc(-0x7b04), 0)};
        for (const auto at : clubs) find(at, {}, {}, "real");
        find(club, plus(base(club), {w32(club + 0x90, 0)}), calm, "no manager");
        find(club, plus(base(club), {w32(club + 0x30, 0)}), {{0x100c7d00, 0}, {0x100db060, 0}}, "no link, refused");
        find(club, plus(plus(base(club), no_clubs), {w32(club + 0x30, 0)}), calm, "no link, supplied");
        find(club, plus(base(club), {w32(manager_record + 0x1c, 0)}), calm, "manager without record");
        // The managers' list: entries that are null, without a +0x1c, or complete.
        for (const std::uint32_t count : {0u, 1u, 3u, 0xffffff80u})
            for (const std::uint32_t kind : {0u, 1u, 2u})
                find(club, plus(plus(base(club), no_clubs), {b8(frame + 0x38, count), w32(frame + 0x3c, kind == 0 ? 0 : entry_at(0)), w32(frame + 0x40, kind == 2 ? entry_at(1) : 0),
                     w32(frame + 0x44, entry_at(2)), w32(entry_at(0) + 0x1c, kind == 1 ? 0 : rating_at(0)), w32(entry_at(1) + 0x1c, rating_at(1)), w32(entry_at(2) + 0x1c, 0),
                     h16(rating_at(0) + 0x2a, 100), h16(rating_at(1) + 0x2a, 0x7fff)}), calm, "list");
        // The three staff members.
        for (unsigned i = 0; i < 3; ++i) {
            const std::uint32_t offset = i == 0 ? 0x74 : i == 1 ? 0x78 : 0x7c;
            for (const std::uint32_t n1 : {0u, 5u, 0xfffffffcu})
                for (const std::uint32_t x1 : {0u, 1u})
                    for (const std::uint32_t n2 : {0u, 3u, 100u})
                        for (const std::uint32_t x2 : {0u, 50u})
                            for (const std::uint32_t equal : {0u, 1u})
                                for (const std::uint32_t gap : {0u, 1u})
                                    for (const std::uint32_t x3 : {0u, 1u, 2u}) {
                                        const auto member = staff_at(i);
                                        const std::vector<Write> memory{w32(club + offset, member), w32(member + 0x90, detail_at(i)), w32(member + 0x30, equal ? link : link + 1),
                                            h16(member + 0x1ec, gap ? 5000 : 1000), w32(detail_at(i), 0x7000 + i), b8(detail_at(i) + 0x68, n1), b8(detail_at(i) + 0x63, n2)};
                                        find(club, plus(plus(plus(base(club), no_clubs), memory), table(scenario, {x1, x2, x3}, 0)), calm, "staff");
                                    }
            // A staff member without a +0x90 record, and the table wrapping at the second and third draw.
            find(club, plus(plus(base(club), no_clubs), {w32(club + offset, staff_at(i)), w32(staff_at(i) + 0x90, 0)}), calm, "staff without record");
            for (const std::uint32_t x : {0u, 50u}) {
                const auto member = staff_at(i);
                const std::vector<Write> memory{w32(club + offset, member), w32(member + 0x90, detail_at(i)), w32(member + 0x30, link), h16(member + 0x1ec, 5000),
                                                w32(detail_at(i), 0x7000 + i), b8(detail_at(i) + 0x68, 5), b8(detail_at(i) + 0x63, 100)};
                find(club, plus(plus(plus(base(club), no_clubs), memory), table(scenario, {x, x, 1}, 0x80000001u, true)), calm, "staff, wrapping");
            }
        }
        // The draw over other clubs: the count n = clubs - 2 * other (zero, small, negative, beyond 16 bits).
        const auto candidate = [&](unsigned i) { return scenario.club(i); };
        for (const auto& [total, other] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0, 0}, {2, 0}, {1, 0}, {3, 5}, {100000, 0}, {70000, 1}, {0x7fffffff, 0}, {0x80000000u, 0}, {0x1999999a, 0}})
            for (const std::uint32_t accepted : {0u, 1u})
                for (const std::uint32_t n1 : {0u, 5u, 0xfffffffcu})
                    for (const std::uint32_t n2 : {0u, 3u, 100u})
                        for (const std::uint32_t x : {0u, 50u, 7u}) {
                            std::vector<Write> memory{w32(scenario.toc(-0x7b50), total), w32(scenario.toc(-0x7b04), other)};
                            for (unsigned i = 0; i < 2; ++i) {
                                memory.push_back(w32(candidate(i) + 0x90, detail_at(i) - 0));
                                memory.push_back(w32(detail_at(i), 0x7100 + i));
                                memory.push_back(b8(detail_at(i) + 0x68, i ? n2 : n1));
                                memory.push_back(b8(detail_at(i) + 0x63, i ? n1 : n2));
                            }
                            find(club, plus(plus(base(club), memory), table(scenario, {x, x ^ 1, x, 50, 50, 50, 50, 50}, 0)), {{0x100db060, 0}, {0x100c7d00, 1}, {0x10464160, accepted}}, "clubs");
                        }
    }

    // The table wrapping at each later draw (cursor `skip` steps below the end), so every refill site is reached.
    {
        const std::uint32_t standing = scratch_base + 0x180, list = scratch_base + 0x1c0, owner_a = scratch_base + 0x400, record = scratch_base + 0x800;
        const std::uint32_t manager_record = scratch_base + 0x5000, link = 0x1234, kept = scratch_base + 0x3000;
        // 0x10647c00: the finer draw's step.
        for (const std::uint32_t n : {100000u, 3000000u, 214748364u, 0xfffe7960u})
            cases.push_back({.entry = 0x10647c00, .registers = scratch_registers(n), .memory = table(scenario, {1000, 5}, 0x1fff, true, 1), .f1 = {}, .label = "wraps late"});
        // 0x106548e0: the gate's draw (skip 0), the kind's draw after it (skip 1), and the kind's draw alone.
        for (std::int32_t kind = 0; kind <= 0x26; ++kind)
            for (const std::uint32_t morale : {0u, 10u})
                for (const unsigned skip : {0u, 1u}) {
                    const std::vector<Write> memory = joined_writes(table(scenario, {5, 1, 2, 7}, 0, true, skip), {w32(standing + 0x50, skip ? 8 : 0), b8(person + 0x69, 0), b8(contract(person) + 0x44, morale)});
                    call(0x106548e0, args({{3, list}, {4, person}, {5, club}, {6, std::uint32_t(kind)}, {7, 0x1ff}}), joined_writes(memory, {w32(list, 0), w32(list + 8, 0)}), {{0x10082f90, standing}, {0x1018c340, 0}}, "wraps");
                }
        // 0x10647730: the count's draw, the weight's draw and the stranger test.
        for (const unsigned skip : {0u, 1u, 2u, 3u, 4u})
            for (const std::uint32_t rating : {7000u, 20u}) {
                const std::vector<Write> memory = joined_writes(table(scenario, {3, 50, 70, 1, 2, 3}, 0, true, skip),
                    {w32(scenario.toc(-0x7be4), 2), b8(scenario.global(-0x7be0) + 0x5e, 60), h16(scenario.global(-0x7be0) + 0x5c, 0x30), w32(scenario.global(-0x7be0) + 0x1c, record),
                     w32(scenario.global(-0x7be0) + 0x14, 0), w32(club + 0x90, 0), w32(scenario.global(-0x7be0) + 0xc, owner_a), h16(owner_a + 0xac, rating), h16(record + 0x2c, 7000),
                     w32(club + 0x2c, 0xfeed), w32(people[1] + 0xc, scratch_base + 0x600)});
                call(0x10647730, args({{4, club}, {5, people[1]}}), memory, {{0x100d9580, 0}}, "wraps");
            }
        // 0x106487d0: each staff member's three draws, and the clubs' three.
        const auto detail = scratch_base + 0x4000;
        for (const unsigned skip : {0u, 1u, 2u, 3u})
            for (unsigned i = 0; i < 3; ++i) {
                const std::uint32_t offset = i == 0 ? 0x74 : i == 1 ? 0x78 : 0x7c, member = kept + i * 0x200;
                const std::vector<Write> memory = joined_writes(table(scenario, {0, 50, 1, 1}, 0x80000001u, true, skip),
                    {w32(club + 0x90, manager_record), w32(manager_record + 0x1c, 1), w32(club + 0x30, link), h16(club + 0x1ec, 1000), w32(club + 0x74, 0), w32(club + 0x78, 0), w32(club + 0x7c, 0),
                     w32(club + offset, member), w32(member + 0x90, detail + i * 0x100), w32(member + 0x30, link), h16(member + 0x1ec, 5000), b8(detail + i * 0x100 + 0x68, 5),
                     b8(detail + i * 0x100 + 0x63, 100), w32(scenario.toc(-0x7b50), 0), w32(scenario.toc(-0x7b04), 0), b8((std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(1))) - 0xe0) + 0x38, 0)});
                call(0x106487d0, args({{4, club}}), memory, {{0x100db060, 0}, {0x100c7d00, 1}, {0x10464160, 0}}, "wraps");
            }
        for (const unsigned skip : {0u, 1u, 2u, 3u, 4u})
            for (const std::uint32_t total : {2u, 100000u}) {
                std::vector<Write> memory = joined_writes(table(scenario, {1, 0, 0, 50, 50, 50}, 0x80000001u, true, skip),
                    {w32(club + 0x90, manager_record), w32(manager_record + 0x1c, 1), w32(club + 0x30, link), w32(club + 0x74, 0), w32(club + 0x78, 0), w32(club + 0x7c, 0),
                     w32(scenario.toc(-0x7b50), total), w32(scenario.toc(-0x7b04), 0), b8((std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(1))) - 0xe0) + 0x38, 0)});
                for (unsigned i = 0; i < 2; ++i) {
                    memory.push_back(w32(scenario.club(i) + 0x90, detail + i * 0x100));
                    memory.push_back(w32(detail + i * 0x100, 0x7100 + i));
                    memory.push_back(b8(detail + i * 0x100 + 0x68, 5));
                    memory.push_back(b8(detail + i * 0x100 + 0x63, 100));
                }
                call(0x106487d0, args({{4, club}}), memory, {{0x100db060, 0}, {0x100c7d00, 1}, {0x10464160, 1}}, "wraps");
            }
        // 0x1066b600 / 0x1066bc50 are covered by their draw cases (skip 0).
    }

    // Each test of the ability bytes failing on its own (0x1067d830), the +0x24 link missing (0x10647730), the pool's
    // equal-ratio case (0x1066bc50) and a candidate club without a manager (0x106487d0).
    {
        const std::uint32_t record = scratch_base + 0x100;
        for (const std::uint32_t b65 : {5u, 18u})
            for (const std::uint32_t b63 : {16u, 4u})
                for (const std::uint32_t b69 : {3u, 12u})
                    for (const std::uint32_t b67 : {5u, 12u})
                        for (const std::uint32_t b5c : {0x30u, 0x10u})
                            for (const std::uint32_t flags : {8u, 0x181u})
                                for (const std::uint32_t grade : {2u, 3u}) {
                                    const std::vector<Write> memory{w32(record + 0x50, flags), h16(contract(person) + 0xa, 5000), h16(club + 0x1ec, 100), b8(person + 0x65, b65),
                                        b8(person + 0x63, b63), b8(person + 0x69, b69), b8(person + 0x67, b67), h16(person + 0x5c, b5c)};
                                    call(0x1067d830, args({{4, person}, {5, club}, {6, 5}, {7, grade}}), memory, {{0x10082f90, record}, {0x1007c140, 1}, {0x10718130, 0x400}}, "bytes");
                                }
        const auto first = scenario.global(-0x7be0), owner_a = scratch_base + 0x400, link_record = scratch_base + 0x800;
        call(0x10647730, args({{4, club}, {5, 0}}),
             {w32(scenario.toc(-0x7be4), 1), b8(first + 0x5e, 60), h16(first + 0x5c, 0x30), w32(first + 0x1c, link_record), w32(first + 0x14, link_record), w32(first + 0x24, 0),
              w32(club + 0x90, 0), w32(first + 0xc, owner_a), h16(owner_a + 0xac, 7000), w32(club + 0x2c, owner_a)}, {}, "linked, no +0x24");
        const std::uint32_t world = scenario.global(-0x7b3c), equal_at = scenario.toc(-0x41fc) + 0x28;
        {
            const std::vector<Write> memory{w32(world + 0x18, scenario.r32(equal_at)), w32(world + 0x1c, scenario.r32(equal_at + 4)), w32(scratch_base + 0x2c0 + 0x60, 1)};
            call(0x1066bc50, args({{4, club}, {5, people[0]}, {6, 1}, {7, 900}, {8, 2}, {9, 0}}), memory,
                 {{0x100d9520, 0}, {0x100d9580, 0}, {0x10082f20, scratch_base + 0x2c0}, {0x10303090, 5}, {0x10718130, 0x5dd}, {0x10084be0, 0x44aa21}}, "equal ratio");
        }
        const std::uint32_t manager_record = scratch_base + 0x5000, detail = scratch_base + 0x4000;
        const auto asker = clubs[2];
        for (const std::uint32_t accepted : {0u, 1u}) {
            std::vector<Write> memory{w32(asker + 0x90, manager_record), w32(manager_record + 0x1c, 1), w32(asker + 0x30, 0x1234), w32(asker + 0x74, 0), w32(asker + 0x78, 0), w32(asker + 0x7c, 0),
                w32(scenario.toc(-0x7b50), 2), w32(scenario.toc(-0x7b04), 0), b8((std::uint32_t(scenario.s.r<4>(cm::aot::guest::reg::gpr(1))) - 0xe0) + 0x38, 0),
                w32(scenario.club(0) + 0x90, 0), w32(scenario.club(1) + 0x90, detail), w32(detail, 0x7100), b8(detail + 0x68, 5), b8(detail + 0x63, 100)};
            call(0x106487d0, args({{4, asker}}), joined_writes(memory, table(scenario, {0, 1, 0, 50, 50, 50}, 0)), {{0x100db060, 0}, {0x100c7d00, 1}, {0x10464160, accepted}}, "club without manager");
        }
    }
}
