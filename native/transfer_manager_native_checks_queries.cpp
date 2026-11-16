// Cases for transfer_manager_native_queries*.cpp: the offer queries, fee checks and the window dates.
#include "guest_abi.hpp"
#include "transfer_manager_native_checks.hpp"
#include <algorithm>

namespace cm::aot::transfer_manager_native_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t reason_cell = scratch_base + 0x100, club_cell = scratch_base + 0x140, fake_person = scratch_base + 0x400;

// Up to `limit` entries of 0..count-1 spread over the whole range, always including the ends.
std::vector<unsigned> spread(unsigned count, unsigned limit) {
    std::vector<unsigned> picks;
    if (count == 0) return picks;
    const auto step = std::max(1u, count / limit);
    for (unsigned index = 0; index < count; index += step) picks.push_back(index);
    if (picks.back() != count - 1) picks.push_back(count - 1);
    return picks;
}
// The manager's offers: pointers of the array at +0 (count at +4), spread.
std::vector<std::uint32_t> offers(const Scenario& scenario, unsigned limit) {
    const auto manager = scenario.transfer_manager();
    const auto count = scenario.r32(manager + 4);
    std::vector<std::uint32_t> result;
    if (count > 100000) return result;
    for (const auto index : spread(count, limit)) {
        if (const auto offer = scenario.r32(scenario.r32(manager) + index * 4)) result.push_back(offer);
    }
    return result;
}
std::string label(const char* name, unsigned a, unsigned b = 0, unsigned c = 0) {
    return std::string(name) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c);
}
// A rule answered by the checker never runs the vector bridge that stores the table of contents in the caller's frame
// (the original reloads r2 from there afterwards): the slot of each frame size the routine can call from is set before
// the answered dispatch, on both sides.
std::vector<Case::Poke> toc_pokes(const Scenario& scenario, std::initializer_list<std::uint32_t> frame_sizes) {
    const auto sp = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    const auto toc = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(2)));
    Case::Poke poke{0x10781320, {}};
    for (const auto size : frame_sizes) poke.writes.push_back({4, sp - size + 0x14, toc});
    return {poke};
}

// What a case adds so that 0x103f83c0 ends with a given reason (0 for success): memory, the callees' answers and
// the first steps of the manager's list walk. `ok` is false when the person's data cannot show that reason.
struct Recipe {
    std::vector<Write> memory;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
    std::vector<Case::NthAnswer> nth;
    bool ok = true;
};
Recipe refusal_recipe(const Scenario& scenario, unsigned reason, std::uint32_t person, std::uint32_t other, std::uint32_t offer) {
    const auto manager = scenario.transfer_manager();
    const auto row = scenario.r32(manager + 0x9a0) + scenario.r32(person) * 10;
    const auto club = scenario.r32(person + 0xc);
    const std::uint32_t holder = scratch_base + 0x4200;
    // Everything earlier than the reason asked for is answered "no": no pending item, a clear status row, nobody listed
    // and no offers; the late checks are set per reason.
    Recipe recipe;
    recipe.answers = {{0x103eb020, 0}, {0x103eb1f0, 0}, {0x1007f7b0, 0}, {0x10738e00, 0}, {0x103f5e50, 0}, {0x100ca800, 0}, {0x10411e10, 0}, {0x100cab10, 0}};
    recipe.memory.push_back({1, row, (scenario.r32(row) >> 24) & 0xbf});
    const auto answer = [&](std::uint32_t entry, std::uint32_t value) {
        for (auto& item : recipe.answers) if (item.first == entry) item.second = value;
    };
    switch (reason) {
    case 0x27:
        if (club == 0) recipe.ok = false;
        recipe.memory.push_back({4, person + 0x24, 0});
        recipe.memory.push_back({4, club + 0x68, scenario.global(-0x7588)});
        recipe.memory.push_back({4, other + 0x2c, scenario.r32(club + 0x2c) + 1});
        break;
    case 0x21:
        recipe.memory.push_back({1, row, (scenario.r32(row) >> 24) | 0x40});
        recipe.memory.push_back({4, scratch_base + 0x4204, 2});
        answer(0x1007f7b0, scratch_base + 0x4200);
        break;
    case 7: answer(0x103eb020, 1); break;
    case 1:
        recipe.memory.push_back({4, scratch_base + 0x4240, scenario.r32(person)});
        recipe.answers.erase(std::remove_if(recipe.answers.begin(), recipe.answers.end(), [](auto& item) { return item.first == 0x10738e00; }), recipe.answers.end());
        recipe.nth.push_back({0x10738e00, 1, scratch_base + 0x4240});
        recipe.answers.push_back({0x10738e00, 0});
        break;
    case 0x12: case 0x20:
        if (offer == 0) recipe.ok = false;
        // The manager's own list is empty (first step answered) and the person's offers are walked for real.
        recipe.answers.erase(std::remove_if(recipe.answers.begin(), recipe.answers.end(), [](auto& item) { return item.first == 0x10738e00; }), recipe.answers.end());
        recipe.nth.push_back({0x10738e00, 1, 0});
        recipe.memory.push_back({1, offer + 0x34, reason == 0x12 ? 0xau : 0x13u});
        answer(0x103f5e50, 1);
        break;
    case 0x19: answer(0x100ca800, 1); answer(0x100cab10, 1); break;
    case 0x28: answer(0x100cab10, 1); break;
    case 0x1a: {
        const auto buyer_club = scenario.club(2);
        recipe.memory.push_back({4, person + 0xc, buyer_club});
        recipe.memory.push_back({4, buyer_club + 0x68, scenario.global(-0x7a18)});
        recipe.memory.push_back({4, person + 0x10, 0});
        answer(0x10411e10, 1);
        break;
    }
    default: break;
    }
    (void)holder;
    return recipe;
}
std::uint32_t person_of(const Scenario& scenario, std::uint32_t offer) { return scenario.person(scenario.r32(offer + 0xc)); }
}

TRANSFER_MANAGER_NATIVE_CASES("queries") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto sample = offers(scenario, 24);
    // 0x103f4c00, 0x103f5c00, 0x103f5d20: persons with offers and persons spread over the table.
    std::vector<std::uint32_t> persons;
    for (const auto offer : sample) persons.push_back(person_of(scenario, offer));
    for (const auto index : spread(people, 30)) persons.push_back(scenario.person(index));
    unsigned n = 0;
    for (const auto person : persons) {
        ++n;
        cases.push_back({.entry = 0x103f4c00, .registers = {{3, manager}, {4, person}}, .label = label("open offer", n)});
        // The club to look for: the one of the person's offers, then a club with none.
        for (const auto offer : sample) {
            cases.push_back({.entry = 0x103f5c00, .registers = {{3, manager}, {4, person}, {5, club_cell}},
                             .memory = {{4, club_cell, scenario.r32(offer + 8)}}, .label = label("offer from club", n, scenario.r32(offer + 8))});
            for (const std::uint32_t state : {0u, 1u, 0x100u})
                cases.push_back({.entry = 0x103f5d20, .registers = {{3, manager}, {4, person}, {5, club_cell}, {6, state}},
                                 .memory = {{4, club_cell, scenario.r32(offer + 8)}}, .label = label("offer record", n, state)});
        }
        cases.push_back({.entry = 0x103f5c00, .registers = {{3, manager}, {4, person}, {5, club_cell}},
                         .memory = {{4, club_cell, 0x7fffffff}}, .label = label("no offer from club", n)});
        cases.push_back({.entry = 0x103f5d20, .registers = {{3, manager}, {4, person}, {5, club_cell}, {6, 1}},
                         .memory = {{4, club_cell, 0x7fffffff}}, .label = label("no offer record", n)});
    }
    // 0x103f4c00: an offer of each kind in the person's list (the has-offers test answered yes).
    for (const auto offer : sample)
        for (const std::uint32_t kind : {0u, 1u, 2u, 5u, 6u, 7u, 0x80u})
            cases.push_back({.entry = 0x103f4c00, .registers = {{3, manager}, {4, person_of(scenario, offer)}}, .memory = {{1, offer + 0x34, kind}},
                             .label = label("open offer kind", kind), .answers = {{0x103f5e50, 1}}});
    // A person record whose id is beyond the table (refused before the status row is read).
    cases.push_back({.entry = 0x103f5c00, .registers = {{3, manager}, {4, fake_person}, {5, club_cell}},
                     .memory = {{4, fake_person, people + 3}, {4, club_cell, 1}}, .label = "offer from club, person beyond table"});
    cases.push_back({.entry = 0x103f5d20, .registers = {{3, manager}, {4, fake_person}, {5, club_cell}, {6, 0}},
                     .memory = {{4, fake_person, people + 3}, {4, club_cell, 1}}, .label = "offer record, person beyond table"});
}

// 0x103f4d10, 0x103f5a50, 0x103f80f0: the fee refusal, the staff offer reasons and the status row ageing.
TRANSFER_MANAGER_NATIVE_CASES("queries") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto sample = offers(scenario, 20);
    unsigned n = 0;
    // 0x103f4d10: the real offers, then the contract fee paths forced through the routines it asks.
    for (std::size_t i = 0; i < sample.size(); ++i) {
        const auto offer = sample[i], competing = sample[(i + 1) % sample.size()];
        for (const auto rival : {0u, competing})
            for (const auto reason : {0u, reason_cell})
                cases.push_back({.entry = 0x103f4d10, .registers = {{4, offer}, {5, rival}, {6, reason}}, .label = label("fee real", i, rival != 0, reason != 0)});
        // The staff path: the lookup says staff, the offer has no list and no flag, then the money compare.
        for (const std::uint32_t fee : {0u, 0x7fffffffu, 0x80000000u, 0xfffffff0u})
            for (const auto reason : {0u, reason_cell})
                cases.push_back({.entry = 0x103f4d10, .registers = {{4, offer}, {5, 0}, {6, reason}},
                                 .memory = {{4, offer + 4, 0}, {1, offer + 0x51, 0}, {4, offer + 0x20, fee}},
                                 .label = label("fee staff", i, fee & 0xff, reason != 0), .answers = {{0x10380520, 1}}});
        // The money paths: budget record with the flag byte, fee of the contract and values of the offers.
        for (const std::uint32_t flag : {0u, 1u})
            for (const std::uint32_t fee : {1000u, 30000u, 200000u, 0x7fffffffu})
                for (const std::uint32_t value : {10000u, 25000u, 60000u, 900000u, 0x40000000u, 0x80000000u})
                    for (const std::uint32_t other : {1u, 100000u, 0x7fffffffu, 0x80000000u})
                        for (const auto rival : {0u, competing}) {
                            if (n++ % 4 != i % 4) continue;  // a quarter of the product per offer
                            cases.push_back({.entry = 0x103f4d10, .registers = {{4, offer}, {5, rival}, {6, reason_cell}},
                                             .memory = {{1, scratch_base + 0x1000 + 0x165, flag}},
                                             .label = label("fee money", i, flag, fee ^ value ^ other),
                                             .answers = {{0x10103510, scratch_base + 0x1000}, {0x10085860, fee}},
                                             .nth_answers = {{0x10416840, 1, value}, {0x10416840, 2, other}}});
                        }
    }
    // 0x103f5a50: reasons of the staff-offer check for each reason bit the callee can return.
    const auto people = scenario.people();
    std::vector<std::uint32_t> profiled;
    for (const auto index : spread(people, 60))
        if (scenario.r32(scenario.person(index) + 0x14) != 0) profiled.push_back(scenario.person(index));
    unsigned k = 0;
    for (const auto offer : std::vector<std::uint32_t>{0, sample.empty() ? 0 : sample[0], sample.size() > 3 ? sample[3] : 0, sample.size() > 7 ? sample[7] : 0})
        for (const auto person : profiled) {
            // No offer: the value lookup would read address 0, so its answer is given.
            std::vector<std::pair<std::uint32_t, std::uint32_t>> no_offer;
            if (offer == 0) no_offer = {{0x10416840, 777}};
            if (k++ % 3 != 0) continue;
            for (const std::uint32_t reasons : {0x2000u, 0x800u, 0x200u, 0x2a00u, 0xa00u, 0x1u, 0u})
                for (const auto same : {true, false})
                    for (const auto reason : {0u, reason_cell})
                        cases.push_back({.entry = 0x103f5a50, .registers = {{4, offer}, {5, scenario.club(2)}, {7, person}, {8, scenario.club(same ? 2 : 5)}, {9, reason}},
                                         .label = label("staff reasons", k, reasons, same), .answers = [&] { auto list = no_offer; list.push_back({0x101a0f50, reasons}); return list; }()});
        }
    // The offer's own list or word making the call skip (offer + 4 pointing at a halfword).
    for (const auto offer : sample)
        for (const std::uint32_t at : {0u, scratch_base + 0x2000u, scratch_base + 0x2004u})
            if (!profiled.empty())
                cases.push_back({.entry = 0x103f5a50, .registers = {{4, offer}, {5, scenario.club(1)}, {7, profiled[0]}, {8, scenario.club(1)}, {9, reason_cell}},
                                 .memory = {{4, offer + 4, at}, {2, scratch_base + 0x2000, 0}, {2, scratch_base + 0x2004, 7}},
                                 .label = label("staff list", at & 0xff), .answers = {{0x101a0f50, 0x2000}}});
    // 0x103f80f0: persons' status rows and dates around the stored one (more than 8 days and past 16 as well).
    for (const auto index : spread(people, 40)) {
        const auto person = scenario.person(index);
        const auto row = scenario.r32(manager + 0x9a0) + scenario.r32(person) * 10;
        const auto stored = scenario.r32(row + 6) >> 16;
        for (const int delta : {-3, 0, 1, 2, 7, 8, 9, 10, 15, 16, 17, 20, 40, 300, 32767, 40000})
            cases.push_back({.entry = 0x103f80f0, .registers = {{3, manager}, {4, person}, {5, (stored + std::uint32_t(delta)) & 0xffff}},
                             .label = label("status row", index, std::uint32_t(delta))});
        cases.push_back({.entry = 0x103f80f0, .registers = {{3, manager}, {4, person}, {5, 0xabcd0000u | (stored + 9)}}, .label = label("status row high bits", index)});
        // A history with the top bits set shifts them out.
        cases.push_back({.entry = 0x103f80f0, .registers = {{3, manager}, {4, person}, {5, stored + 3}},
                         .memory = {{2, row + 4, 0xf0f1}}, .label = label("status row history", index)});
    }
}

// 0x103f83c0: the offer test, each refusal forced through the memory and the routines it consults.
TRANSFER_MANAGER_NATIVE_CASES("queries") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto sample = offers(scenario, 24);
    const std::uint32_t kept_club_word = scenario.global(-0x7588), clubs[] = {scenario.club(0), scenario.club(1), scenario.club(7), scenario.club(60),
        scenario.club(scenario.global(-0x7724)), scenario.club(scenario.global(-0x7864)), scenario.club(200), scenario.club(800)};
    unsigned n = 0;
    const auto add = [&](std::uint32_t person, std::uint32_t other, std::uint32_t flag, std::uint32_t reason, const char* what,
                         std::vector<Write> memory = {}, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {},
                         std::vector<Case::NthAnswer> nth = {}) {
        cases.push_back({.entry = 0x103f83c0, .registers = {{3, manager}, {4, person}, {5, other}, {6, reason}, {7, flag}},
                         .memory = std::move(memory), .label = label(what, n++, flag, reason != 0), .answers = std::move(answers),
                         .nth_answers = std::move(nth)});
    };
    const auto row_of = [&](std::uint32_t person) { return scenario.r32(manager + 0x9a0) + scenario.r32(person) * 10; };
    // The real state: persons spread, the clubs, both flags.
    for (const auto index : spread(people, 40)) {
        const auto person = scenario.person(index);
        for (const auto other : {0u, clubs[0], clubs[3], clubs[4], clubs[5], clubs[7]})
            for (const std::uint32_t flag : {0u, 1u}) add(person, other, flag, (index & 1) ? reason_cell : 0, "offer test real");
    }
    for (const auto offer : sample) {
        const auto person = person_of(scenario, offer);
        // The person's own offers decide: each kind the walk looks for, poked into the offer.
        for (const std::uint32_t kind : {0x0u, 0x5u, 0xau, 0xbu, 0xcu, 0xdu, 0xeu, 0xfu, 0x10u, 0x13u, 0x80u})
            for (const auto other : {0u, clubs[1]})
                add(person, other, 1, reason_cell, "offer test kinds", {{1, offer + 0x34, kind}}, {{0x103f5e50, 1}});
        for (const auto other : {clubs[0], clubs[4]}) add(person, other, 0, reason_cell, "offer test offers", {}, {});
    }
    for (const auto index : spread(people, 12)) {
        const auto person = scenario.person(index);
        const auto club = scenario.r32(person + 0xc);
        const auto id = scenario.r32(person);
        // 0x27: the person's club keeps him (its word at +0x68 is the kept value) and the other club is of another nation.
        for (const auto holder : {0u, scenario.club(3)})
            add(person, clubs[1], 0, reason_cell, "offer test kept", {{4, person + 0x24, holder}, {4, clubs[1] + 0x2c, 0x7ffff001}, {4, clubs[1] + 0x68, 0x1}},
                {}, {});
        if (club != 0) {
            for (const auto other : {clubs[1], clubs[2]}) {
                add(person, other, 0, reason_cell, "offer test kept club", {{4, club + 0x68, kept_club_word}, {4, person + 0x24, 0}, {4, other + 0x2c, scenario.r32(club + 0x2c) + 1}}, {}, {});
                add(person, other, 0, reason_cell, "offer test kept same", {{4, club + 0x68, kept_club_word}, {4, person + 0x24, club - 0x200}, {4, club - 0x200 + 0x2c, 5}}, {}, {});
            }
        }
        // 0x21: the status row says blocked and the contract lookup agrees, or not (the flag is then cleared).
        for (const std::uint32_t found : {0u, 1u})
            add(person, clubs[1], found, reason_cell, "offer test blocked", {{1, row_of(person), scenario.r32(row_of(person)) >> 24 | 0x40}}, {{0x1007f7b0, found}});
        // 7: a pending item found by either routine, only with the flag clear.
        for (const std::uint32_t flag : {0u, 1u, 0x100u})
            for (const std::uint32_t first : {0u, 1u})
                add(person, clubs[1], flag, reason_cell, "offer test pending", {}, {{0x103eb020, first}, {0x103eb1f0, first ? 0u : 1u}});
        // 1: the manager's list names this person (the first step of the list walk is answered).
        for (const std::uint32_t listed : {id, id + 1})
            add(person, clubs[1], 1, reason_cell, "offer test listed", {{4, scratch_base + 0x200, listed}}, {}, {{0x10738e00, 1, scratch_base + 0x200}});
        add(person, clubs[1], 1, reason_cell, "offer test list empty", {}, {}, {{0x10738e00, 1, 0}});
        // The late refusals 0x19, 0x28 and 0x1a, the final success, through the callees' answers.
        for (const auto other : {clubs[0], clubs[4], clubs[5]})
            for (const std::uint32_t a : {0u, 1u})
                for (const std::uint32_t b : {0u, 1u})
                    for (const std::uint32_t c11 : {0u, 1u})
                        add(person, other, 1, reason_cell, "offer test late", {}, {{0x100ca800, a}, {0x100cab10, b}, {0x10411e10, c11}}, {{0x103f5e50, 1, 0}});
        // The 0x1a chain: the other club's id is one of the two special words, the person's club and holder from the table.
        for (const auto other : {clubs[4], clubs[5]})
            for (const auto club_word : {kept_club_word, kept_club_word + 1})
                for (const auto holder_word : {0u, 5u, 9u}) {
                    const auto holder = scenario.club(4);
                    add(person, other, 1, reason_cell, "offer test chain",
                        {{4, person + 0xc, club == 0 ? scenario.club(2) : club}, {4, (club == 0 ? scenario.club(2) : club) + 0x68, scenario.global(-0x7a18) + (club_word - kept_club_word)},
                         {4, person + 0x10, holder_word == 0 ? 0 : holder}, {4, holder + 0x68, holder_word == 5 ? scenario.global(-0x6678) : 3}},
                        {{0x100ca800, 0}, {0x10411e10, 1}, {0x10738e00, 0}}, {{0x103f5e50, 1, 0}});
                }
    }
}

// 0x103f8cc0, 0x103fa290, 0x103fa8e0, 0x103fb130: the window dates and the nation-rule checks over them.
TRANSFER_MANAGER_NATIVE_CASES("queries") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto sample = offers(scenario, 24);
    const auto now = scenario.toc(-0x7b80);
    const auto sink = scenario.toc(-0x7bb8);
    const std::uint32_t now_day = scenario.r32(now) >> 16, now_year = scenario.r32(now) & 0xffff;
    constexpr std::uint32_t out = scratch_base + 0x300, out_end = scratch_base + 0x340;
    unsigned n = 0;
    // 0x103fb130: the clock moved over the season's marks.
    for (int delta = -400; delta <= 400; delta += 7)
        for (const auto end : {0u, out_end})
            cases.push_back({.entry = 0x103fb130, .registers = {{4, out}, {5, end}},
                             .memory = {{2, now, (now_day + std::uint32_t(delta)) & 0xffff}}, .label = label("window clock", std::uint32_t(delta + 400), end != 0)});
    for (std::uint32_t day = 0; day <= 400; day += 5)
        for (const auto end : {0u, out_end})
            cases.push_back({.entry = 0x103fb130, .registers = {{4, out}, {5, end}}, .memory = {{2, now, day}}, .label = label("window day", day, end != 0)});
    // 0x103f8cc0: the earliest offer date, per offer kind nibble and per answer of the nation rules.
    for (std::size_t i = 0; i < sample.size(); ++i) {
        const auto offer = sample[i];
        const auto kind_byte = std::uint32_t(scenario.s.memory.read<1>(offer + 0x38));
        for (const std::uint32_t nibble : {0u, 4u, 5u, 7u}) {
            const Write kind{1, offer + 0x38, (nibble << 4) | (kind_byte & 0xf)};
            cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, out}}, .memory = {kind}, .label = label("first date real", i, nibble)});
            for (const std::uint32_t answer : {0u, 1u})
                cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, out}}, .memory = {kind},
                                 .label = label("first date rules", i, nibble, answer), .answers = {{0x10781320, answer}}, .pokes = toc_pokes(scenario, {0xb0})});
            // The first rule passes, the second refuses (or the other way round), at several days.
            for (const unsigned nth : {1u, 2u, 3u, 40u})
                cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, out}}, .memory = {kind},
                                 .label = label("first date nth", i, nibble, nth), .answers = {{0x10781320, 0}}, .nth_answers = {{0x10781320, nth, 1}, {0x10781320, nth + 1, 1}},
                                 .pokes = toc_pokes(scenario, {0xb0})});
        }
        // The person's club unknown (no profile record): the date is today at once.
        cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, out}}, .memory = {{4, person_of(scenario, offer) + 0x14, 0}, {1, offer + 0x38, 0x10}},
                         .label = label("first date no profile", i)});
        // The contract record of the person decides the earliest date: a contract running past today.
        for (const std::int32_t years : {-1, 0, 1})
            for (const std::int32_t days : {-30, 0, 30})
                cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, out}},
                                 .memory = {{1, offer + 0x38, 0x40 | (kind_byte & 0xf)}, {2, scratch_base + 0x1000 + 0x30, (now_day + std::uint32_t(days)) & 0xffff},
                                            {2, scratch_base + 0x1000 + 0x32, now_year + std::uint32_t(years)}, {1, scratch_base + 0x1000 + 0x34, 0x20}},
                                 .label = label("first date contract", i, std::uint32_t(years + 1), std::uint32_t(days + 30)),
                                 .answers = {{0x10082f20, scratch_base + 0x1000}}});
    }
    // 0x103fa8e0: the window narrowed to the contract's end, over the contract's date and the window's months.
    constexpr std::uint32_t month_a = scratch_base + 0x3000, year_a = scratch_base + 0x3002, month_b = scratch_base + 0x3004,
                            year_b = scratch_base + 0x3006, contract = scratch_base + 0x1800;
    const auto person = scenario.person(5), club = scenario.club(3), club_b = scenario.club(4);
    for (const std::int32_t cy : {-2, -1, 0, 1, 2, 6})
        for (const std::int32_t cd : {-100, -1, 0, 1, 100})
            for (const std::int32_t by : {-1, 0, 1, 5, 6, 8})
                for (const std::uint32_t bm : {0u, 5u, 11u}) {
                    if (n++ % 2) continue;
                    for (const std::int32_t ay : {-3, 2, 9}) {
                        cases.push_back({.entry = 0x103fa8e0,
                                         .registers = {{3, manager}, {4, club}, {5, club_b}, {6, person}, {7, month_a}, {8, year_a}, {9, month_b}, {10, year_b}},
                                         .memory = {{2, contract + 0x30, (now_day + std::uint32_t(cd)) & 0xffff}, {2, contract + 0x32, now_year + std::uint32_t(cy)},
                                                    {1, contract + 0x38, 8}, {1, month_a, 7}, {2, year_a, now_year + std::uint32_t(ay)}, {1, month_b, bm},
                                                    {2, year_b, now_year + std::uint32_t(by)}},
                                         .label = label("narrow", std::uint32_t(cy + 2), std::uint32_t(by + 1), bm),
                                         .answers = {{0x10082f20, contract}, {0x10781320, 0}}, .pokes = toc_pokes(scenario, {0x90})});
                    }
                }
    // The month loop run down below the clock's year: the date built each time is answered as far in the future.
    constexpr std::uint32_t future_date = scratch_base + 0x3100;
    for (const std::int32_t cy : {1, 2})
        for (const std::uint32_t bm : {0u, 4u, 11u})
            for (const std::int32_t by : {0, 1, 3})
                cases.push_back({.entry = 0x103fa8e0,
                                 .registers = {{3, manager}, {4, club}, {5, club_b}, {6, person}, {7, month_a}, {8, year_a}, {9, month_b}, {10, year_b}},
                                 .memory = {{2, contract + 0x30, 200}, {2, contract + 0x32, now_year + std::uint32_t(cy)}, {1, contract + 0x38, 8},
                                            {1, month_a, 7}, {2, year_a, now_year + 9}, {1, month_b, bm}, {2, year_b, now_year + std::uint32_t(by)},
                                            {2, future_date, 100}, {2, future_date + 2, now_year + 50}, {1, future_date + 4, 3}},
                                 .label = label("narrow future", std::uint32_t(cy), bm, std::uint32_t(by)),
                                 .answers = {{0x10082f20, contract}, {0x10781320, 0}, {0x107156f0, future_date}}, .pokes = toc_pokes(scenario, {0x90})});
    // The contract missing, of another type, or the rule table's first entry standing in.
    for (const std::uint32_t type : {0u, 4u, 20u})
        cases.push_back({.entry = 0x103fa8e0,
                         .registers = {{3, manager}, {4, club}, {5, club_b}, {6, person}, {7, month_a}, {8, year_a}, {9, month_b}, {10, year_b}},
                         .memory = {{1, contract + 0x38, type}, {1, month_a, 7}, {2, year_a, now_year}, {1, month_b, 3}, {2, year_b, now_year}},
                         .label = label("narrow type", type), .answers = {{0x10082f20, contract}, {0x10781320, 0}}, .pokes = toc_pokes(scenario, {0x90})});
    for (const std::uint32_t nation : {0x7ffffff1u, 0u})
        cases.push_back({.entry = 0x103fa8e0,
                         .registers = {{3, manager}, {4, club}, {5, club_b}, {6, person}, {7, month_a}, {8, year_a}, {9, month_b}, {10, year_b}},
                         .memory = {{4, club + 0x2c, nation}, {1, contract + 0x38, 0}, {1, month_a, 7}, {2, year_a, now_year}, {1, month_b, 3}, {2, year_b, now_year}},
                         .label = label("narrow nation", nation & 0xff), .answers = {{0x10082f20, 0}, {0x10781320, 0}}, .pokes = toc_pokes(scenario, {0x90})});
    // 0x103fa290: the transfer check for persons and clubs spread over the tables, with and without the text sink.
    unsigned m = 0;
    for (const auto index : spread(people, 60)) {
        const auto who = scenario.person(index);
        for (const auto buyer : {scenario.club(0), scenario.club(7), scenario.club(60)})
            for (const auto text : {0u, sink}) {
                if (m++ % 2) continue;
                cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, buyer}, {5, who}, {6, text}}, .label = label("possible real", index, text != 0)});
                cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, buyer}, {5, who}, {6, text}}, .memory = {{1, who + 0x62, 0xb}},
                                 .label = label("possible type b", index, text != 0)});
            }
    }
    // The branches after the offer test, through the rules' answers: first rule yes, then each month's answer.
    for (const auto index : spread(people, 12)) {
        const auto who = scenario.person(index);
        for (const std::uint32_t month_answer : {0u, 1u})
            for (const unsigned nth : {2u, 3u, 5u, 14u})
                for (const auto text : {0u, sink})
                    cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, scenario.club(0)}, {5, who}, {6, text}},
                                     .memory = {{1, who + 0x62, 0xb}}, .label = label("possible months", index, month_answer, nth),
                                     .answers = {{0x10781320, 0}}, .nth_answers = {{0x10781320, 1, 1}, {0x10781320, nth, month_answer}},
                                     .pokes = toc_pokes(scenario, {0x2f0, 0x2f0 + 0x90})});
    }
}

// 0x103f83c0 and 0x103fa290 (with its explanations and month loop) for each reason, through the recipes.
TRANSFER_MANAGER_NATIVE_CASES("queries") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto sink = scenario.toc(-0x7bb8), now = scenario.toc(-0x7b80);
    const auto sample = offers(scenario, 12);
    const std::uint32_t now_year = scenario.r32(now) & 0xffff;
    const auto sp = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    std::vector<std::uint32_t> persons;
    for (const auto index : spread(scenario.people(), 40))
        if (scenario.r32(scenario.person(index) + 0x14) != 0 && scenario.r32(scenario.person(index) + 0xc) != 0) persons.push_back(scenario.person(index));
    if (persons.size() > 6) persons.resize(6);
    const auto rule0 = scenario.r32(manager + 0x9e0), rule1 = scenario.r32(manager + 0x9e4);
    const std::uint32_t contract = scratch_base + 0x4400, special = scenario.global(-0x7724);
    const auto toc = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(2)));
    unsigned n = 0;
    for (const unsigned reason : {0x27u, 0x21u, 7u, 1u, 0x12u, 0x20u, 0x19u, 0x28u, 0x1au, 0u}) {
        const auto other = reason == 0x1a ? scenario.club(special) : scenario.club(7);
        for (std::size_t i = 0; i < std::max<std::size_t>(persons.size(), reason == 0x12 || reason == 0x20 ? sample.size() : 0); ++i) {
            const bool by_offer = reason == 0x12 || reason == 0x20;
            if (by_offer && i >= sample.size()) break;
            if (!by_offer && i >= persons.size()) break;
            const auto offer = by_offer ? sample[i] : 0u;
            const auto person = by_offer ? person_of(scenario, offer) : persons[i];
            const auto recipe = refusal_recipe(scenario, reason, person, other, offer);
            if (!recipe.ok) continue;
            cases.push_back({.entry = 0x103f83c0, .registers = {{3, manager}, {4, person}, {5, other}, {6, reason_cell}, {7, 0}}, .memory = recipe.memory,
                             .label = label("recipe offer test", reason, n++), .answers = recipe.answers, .nth_answers = recipe.nth});
            for (const auto text : {0u, sink})
                for (const unsigned answered : {1u, 4u, 9u, 30u}) {
                    if (reason != 0 && answered != 1) continue;
                    auto memory = std::vector<Write>{{4, other + 0x2c, scenario.r32(rule0 + 0xc)}, {4, scenario.club(2) + 0x2c, scenario.r32(rule1 + 0xc)},
                                                     {1, person + 0x62, 0xb}, {1, contract + 0x38, 8}, {4, contract + 4, 2}, {2, contract + 0x30, 100},
                                                     {2, contract + 0x32, now_year + 3}, {1, contract + 0x34, 0}};
                    memory.insert(memory.end(), recipe.memory.begin(), recipe.memory.end());
                    auto answers = recipe.answers;
                    answers.push_back({0x10082f20, contract});
                    answers.push_back({0x10083110, 0});
                    auto nth = recipe.nth;
                    // The rules are answered only on the path that reaches them: a callee that restores r2 from its own
                    // stale frame after an answered rule would run with a zero table of contents.
                    if (reason == 0) {
                        answers.push_back({0x10781320, 0});
                        nth.push_back({0x10781320, 1, 1});
                        nth.push_back({0x10781320, answered + 2, 1});
                    }
                    const auto base = sp - 0x2f0;
                    std::vector<Case::Poke> pokes = toc_pokes(scenario, {0x2f0, 0x2f0 + 0x90});
                    pokes.push_back({0x10781320, {{1, base + 0x39, 10}, {2, base + 0x3e, now_year}, {1, base + 0x38, 3}, {2, base + 0x3c, now_year + 1}}});
                    pokes.push_back({0x1007f7b0, {{4, base + 0x40, scenario.club(2)}}});
                    (void)toc;
                    cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, other}, {5, person}, {6, text}}, .memory = memory,
                                     .label = label("recipe possible", reason, n++, answered), .answers = answers, .nth_answers = nth, .pokes = pokes});
                    if (reason == 0x21 && answered == 1) {
                        // The explanation asks the contract lookup again: this time nothing is found and the blocked flag is cleared.
                        auto second = nth;
                        for (auto& item : answers) if (item.first == 0x1007f7b0) item.second = 0;
                        second.push_back({0x1007f7b0, 1, scratch_base + 0x4200});
                        cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, other}, {5, person}, {6, text}}, .memory = memory,
                                         .label = label("recipe possible cleared", reason, n++), .answers = answers, .nth_answers = second, .pokes = pokes});
                    }
                    if (reason == 0 && answered == 1) {
                        // The buyer's nation rule refuses.
                        auto refused = nth;
                        refused.push_back({0x10781320, 1, 0});
                        for (auto& item : refused) if (item.entry == 0x10781320 && item.nth == 1) item.answer = 0;
                        cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, other}, {5, person}, {6, text}}, .memory = memory,
                                         .label = label("recipe possible refused", reason, n++), .answers = answers, .nth_answers = refused, .pokes = pokes});
                        // A person of another type: the explanation is printed (or not) and the check ends.
                        auto other_type = memory;
                        other_type.push_back({1, person + 0x62, 5});
                        cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, other}, {5, person}, {6, text}}, .memory = other_type,
                                         .label = label("recipe possible type", reason, n++), .answers = answers, .nth_answers = nth, .pokes = pokes});
                    }
                }
        }
    }
}

// Gaps closed in one pass: null persons, the 0x27 sub-conditions, the contract lookup answered empty, rule reasons that end the
// date search, and the explanation after the status flag was cleared in between.
TRANSFER_MANAGER_NATIVE_CASES("queries") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto sample = offers(scenario, 12);
    const auto now = scenario.toc(-0x7b80);
    const std::uint32_t now_year = scenario.r32(now) & 0xffff;
    const auto sp = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    const auto sink = scenario.toc(-0x7bb8);
    unsigned n = 0;
    // A null person is read at address 0 on both sides.
    cases.push_back({.entry = 0x103f5c00, .registers = {{3, manager}, {4, 0}, {5, club_cell}}, .memory = {{4, club_cell, 1}}, .label = "null person 5c00"});
    cases.push_back({.entry = 0x103f5d20, .registers = {{3, manager}, {4, 0}, {5, club_cell}, {6, 1}}, .memory = {{4, club_cell, 1}}, .label = "null person 5d20"});
    cases.push_back({.entry = 0x103f83c0, .registers = {{3, manager}, {4, 0}, {5, scenario.club(1)}, {6, reason_cell}, {7, 1}}, .label = "null person 83c0"});
    // 0x103f83c0: the kept-club test with each sub-condition varied.
    for (const auto offer : sample) {
        const auto person = person_of(scenario, offer);
        const auto club = scenario.r32(person + 0xc);
        const std::uint32_t kept = scenario.global(-0x7588), holder = scratch_base + 0x5800;
        const auto other = scenario.club(1);
        std::vector<Write> base{{4, club + 0x68, kept}};
        const auto go = [&](std::vector<Write> memory, std::uint32_t o, const char* what) {
            cases.push_back({.entry = 0x103f83c0, .registers = {{3, manager}, {4, person}, {5, o}, {6, reason_cell}, {7, 1}}, .memory = std::move(memory),
                             .label = label(what, n++)});
        };
        go({}, other, "gap club zero");
        go({{4, person + 0xc, 0}}, other, "gap club zero");
        if (club != 0) {
            go({{4, club + 0x68, kept}, {4, person + 0x24, holder}, {4, holder + 0x2c, club}, {4, other + 0x2c, scenario.r32(club + 0x2c) + 1}}, other, "gap holder same");
            go({{4, club + 0x68, kept}, {4, person + 0x24, holder}, {4, holder + 0x2c, club + 8}, {4, other + 0x2c, scenario.r32(club + 0x2c) + 1}}, other, "gap holder other");
            go({{4, club + 0x68, kept}, {4, person + 0x24, 0}}, 0, "gap no other");
            go({{4, club + 0x68, kept}, {4, person + 0x24, 0}, {4, other + 0x2c, club}}, other, "gap other nation");
        }
        // The chain with the person's club absent.
        go({{4, person + 0xc, 0}, {4, person + 0x10, 0}}, scenario.club(scenario.global(-0x7724)), "gap chain no club");
        cases.back().answers = {{0x100ca800, 0}, {0x10411e10, 1}, {0x103f5e50, 0}, {0x10738e00, 0}};
    }
    // 0x103f8cc0: the contract lookup empty, or a contract of type 5, for each kind nibble.
    for (const auto offer : sample) {
        const auto kind_byte = std::uint32_t(scenario.s.memory.read<1>(offer + 0x38));
        for (const std::uint32_t nibble : {0u, 4u})
            for (const std::uint32_t type : {0u, 0x14u}) {
                cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, scratch_base + 0x300}},
                                 .memory = {{1, offer + 0x38, (nibble << 4) | (kind_byte & 0xf)}, {1, scratch_base + 0x1000 + 0x38, type}, {4, scratch_base + 0x1004, 2},
                                            {2, scratch_base + 0x1030, 100}, {2, scratch_base + 0x1032, now_year + 1}},
                                 .label = label("gap lookup", n++, type), .answers = {{0x10082f20, type ? scratch_base + 0x1000 : 0}}});
            }
    }
    // The date search ended or continued by the reason the rules leave: with and without a second rule (a seller).
    const auto rule0 = scenario.r32(manager + 0x9e0), rule1 = scenario.r32(manager + 0x9e4);
    constexpr std::uint32_t contract = scratch_base + 0x4400;
    for (std::size_t i = 0; i < sample.size() && i < 6; ++i) {
        const auto offer = sample[i];
        const auto buyer = scenario.club(scenario.r32(offer + 8));
        const auto kind_byte = std::uint32_t(scenario.s.memory.read<1>(offer + 0x38));
        for (const std::uint32_t code : {0x14u, 0x15u, 0x1fu, 0x11u, 9u, 5u})
            for (const bool seller : {false, true})
                for (const unsigned buyer_answer : {0u, 1u})
                    for (const unsigned seller_answer : {0u, 1u}) {
                        if (!seller && seller_answer) continue;
                        std::vector<Write> memory{{1, offer + 0x38, kind_byte & 0xf}, {4, buyer + 0x2c, scenario.r32(rule0 + 0xc)},
                                                  {4, scenario.club(2) + 0x2c, scenario.r32(rule1 + 0xc)}, {1, contract + 0x38, 8}, {4, contract + 4, 2},
                                                  {4, offer + 4, seller ? scratch_base + 0x5000 : 0}, {2, scratch_base + 0x5000, 0}};
                        const auto base = sp - 0xb0;
                        std::vector<Case::Poke> pokes = toc_pokes(scenario, {0xb0});
                        pokes.push_back({0x10781320, {{2, base + 0x38, code}}});
                        cases.push_back({.entry = 0x103f8cc0, .registers = {{3, manager}, {4, offer}, {5, scratch_base + 0x300}}, .memory = memory,
                                         .label = label("gap reasons", n++, code), .answers = {{0x10781320, 0}, {0x10082f20, contract}},
                                         .nth_answers = {{0x10781320, 1, buyer_answer}, {0x10781320, 2, seller_answer}}, .pokes = pokes});
                    }
    }
    // 0x103fa290: the explanation of code 0x21 when the status flag was cleared in the meantime (by the person description).
    for (const auto offer : sample) {
        const auto person = person_of(scenario, offer);
        const auto row = scenario.r32(manager + 0x9a0) + scenario.r32(person) * 10;
        const auto recipe = refusal_recipe(scenario, 0x21, person, scenario.club(7), 0);
        auto memory = recipe.memory;
        memory.insert(memory.end(), {{1, person + 0x62, 0xb}, {1, contract + 0x38, 8}, {4, contract + 4, 2}, {2, contract + 0x30, 100}, {2, contract + 0x32, now_year + 3}});
        auto answers = recipe.answers;
        answers.push_back({0x10082f20, contract});
        answers.push_back({0x10083110, 0});
        cases.push_back({.entry = 0x103fa290, .registers = {{3, manager}, {4, scenario.club(7)}, {5, person}, {6, sink}}, .memory = memory,
                         .label = label("gap flag cleared", n++), .answers = answers,
                         .pokes = {{0x100caeb0, {{1, row, (scenario.r32(row) >> 24) & 0xbf}}}}});
    }
}
}
