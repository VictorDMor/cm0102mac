// Cases for transfer_manager_native_approaches*.cpp: 0x103e25d0, 0x103e3170, 0x103e3410, 0x103f3bf0, 0x1040d5c0,
// 0x103e2db0, 0x103e3240, 0x103e3de0.
#include "guest_abi.hpp"
#include "transfer_manager_native_checks.hpp"
#include <algorithm>
#include <array>
#include <string>

namespace cm::aot::transfer_manager_native_checks {
namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t table_of_contents = 0x108bba90, human_entry = 0x1014c2e0, level_entry = 0x105c5030;
std::string label(const char* what, unsigned a, unsigned b = 0, unsigned c = 0, unsigned d = 0) {
    return std::string(what) + " " + std::to_string(a) + "/" + std::to_string(b) + "/" + std::to_string(c) + "/" + std::to_string(d);
}
std::vector<std::pair<unsigned, std::uint32_t>> with_toc(std::vector<std::pair<unsigned, std::uint32_t>> registers) {
    registers.insert(registers.begin(), {2, table_of_contents});
    return registers;
}
std::uint32_t caller_sp(const Scenario& scenario) { return guest::Cpu(scenario.s).sp(); }
// Real offers of the manager (an array of pointers at +0, its size at +4), spread over the array.
std::vector<std::uint32_t> real_offers(const Scenario& scenario, unsigned wanted) {
    std::vector<std::uint32_t> offers;
    const auto manager = scenario.transfer_manager();
    const auto array = scenario.r32(manager), size = scenario.r32(manager + 4);
    if (size == 0 || size > 100000) return offers;
    const auto step = size / wanted + 1;
    for (std::uint32_t i = 0; i < size; i += step)
        if (const auto offer = scenario.r32(array + i * 4)) offers.push_back(offer);
    return offers;
}
// A copy of an offer record in the scratch area with some fields replaced (a word or byte each).
struct Field { unsigned size; std::uint32_t offset, value; };
std::uint32_t scratch_offer(const Scenario& scenario, Writes& writes, std::uint32_t at, std::uint32_t from,
                            std::initializer_list<Field> fields = {}) {
    for (std::uint32_t offset = 0; offset < 0x54; offset += 4) writes.push_back({4, at + offset, scenario.r32(from + offset)});
    for (const auto& field : fields) writes.push_back({field.size, at + field.offset, field.value});
    return at;
}
void write_date(Writes& writes, std::uint32_t at, std::uint32_t year, std::uint32_t day, std::uint32_t extra) {
    writes.push_back({2, at, year});
    writes.push_back({2, at + 2, day});
    writes.push_back({1, at + 4, extra});
    writes.push_back({1, at + 5, 0});
    writes.push_back({2, at + 6, 0});
}
void write_double(Writes& writes, std::uint32_t at, std::uint64_t bits) {
    writes.push_back({4, at, std::uint32_t(bits >> 32)});
    writes.push_back({4, at + 4, std::uint32_t(bits)});
}
// Garbage in the words below the entry stack pointer, where the routine's frame will lie.
Writes stale(const Scenario& scenario, std::uint32_t frame_size, std::uint32_t salt) {
    Writes writes;
    const auto sp = caller_sp(scenario);
    for (std::uint32_t offset = 0x30; offset < frame_size; offset += 4)
        writes.push_back({4, sp - frame_size + offset, 0x5a5a0000u + salt * 0x101 + offset});
    return writes;
}
void append(Writes& to, const Writes& from) { to.insert(to.end(), from.begin(), from.end()); }
}

TRANSFER_MANAGER_NATIVE_CASES("approaches") {
    if (scenario.match) return;
    const auto manager = scenario.transfer_manager();
    const auto people = scenario.people();
    const auto offers = real_offers(scenario, 8);
    const auto today = scenario.toc(-0x7b80);
    const auto year = scenario.r32(today) >> 16, day = scenario.r32(today) & 0xffff;
    const auto scratch = scratch_base + 0x10000;
    std::uint32_t salt = 1;

    // ---- 0x103e25d0: the source is a scratch copy of a real offer: the same club (replaced), another club
    // for the same person, and another person; the person's status row empty or not.
    for (std::size_t i = 0; i < offers.size() && i < 6; ++i) {
        const auto offer = offers[i];
        const auto person = scenario.r32(offer + 0xc), club = scenario.r32(offer + 8);
        const auto at = scratch + std::uint32_t(i) * 0x200;
        const auto rows = scenario.r32(manager + 0x9a0);
        struct Variant { std::uint32_t person, club; };
        const Variant variants[] = {{person, club}, {person, club + 1}, {(person + 7) % people, club}, {(person + 11) % people, club + 3}};
        for (std::size_t v = 0; v < 4; ++v)
            for (const std::uint32_t kind : {0u, 1u})
                for (const std::uint32_t human_flag : {0u, 1u})
                    for (const std::uint32_t answer : {0u, 1u}) {
                        if (!human_flag && answer && v % 2) continue;
                        Writes memory;
                        scratch_offer(scenario, memory, at, offer,
                                      {{4, 0xc, variants[v].person}, {4, 8, variants[v].club}, {1, 0x24, kind}, {1, 0x37, human_flag},
                                       {4, 0x20, (i + v) % 3 ? 5u : 0u}, {1, 0x35, (i + v) % 4 == 0 ? 2u : 1u}});
                        if (v == 3) memory.push_back({1, rows + variants[v].person * 10 + 1, 0});
                        if ((i + v) % 2) append(memory, stale(scenario, 0xd0, salt++));
                        Case one{.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offer}}), .memory = memory,
                                 .label = label("add offer", unsigned(i), unsigned(v), kind * 4 + human_flag * 2 + answer)};
                        one.answers.push_back({human_entry, answer});
                        cases.push_back(one);
                    }
        // The list of the offer (people of a bidder list) present or not.
        {
            Writes memory;
            scratch_offer(scenario, memory, at, offer, {{4, 0x24, 1}, {4, 4, scenario.r32(offer + 4)}});
            cases.push_back({.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offer}}), .memory = memory,
                             .label = label("add offer list", unsigned(i))});
        }
    }
    // The table full (growth through the realloc, or none yet), an allocation failure and no free slot at all.
    if (!offers.empty()) {
        const auto offer = offers[0];
        const auto at = scratch + 0x3000;
        const auto size = scenario.r32(manager + 4);
        Writes base;
        scratch_offer(scenario, base, at, offer, {{4, 0xc, (scenario.r32(offer + 0xc) + 5) % people}});
        Writes grown = base;
        grown.push_back({4, manager + 8, size});
        cases.push_back({.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offer}}), .memory = grown, .label = "add offer grown"});
        Writes empty = grown;
        empty.push_back({4, manager, 0});
        cases.push_back({.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offer}}), .memory = empty, .label = "add offer first table"});
        // A table too large to grow: the allocator refuses (an answer for it would not reach the original,
        // which runs the allocator in place).
        Writes huge = base;
        huge.push_back({4, manager + 4, 0x30000000});
        huge.push_back({4, manager + 8, 0x30000000});
        cases.push_back({.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offer}}), .memory = huge, .label = "add offer realloc fails"});
        // Every entry taken and the count unequal to the size: no free slot.
        Writes full = base;
        full.push_back({4, manager + 8, size + 1});
        const auto array = scenario.r32(manager);
        for (std::uint32_t k = 0; k < size && k < 4000; ++k)
            if (!scenario.r32(array + k * 4)) full.push_back({4, array + k * 4, offer});
        cases.push_back({.entry = 0x103e25d0, .registers = with_toc({{3, manager}, {4, at}, {5, offer}}), .memory = full, .label = "add offer no slot"});
    }

    // ---- 0x103e3170: person and club of a real offer, the term words as the callers give them.
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto person = scenario.person(scenario.r32(offers[i] + 0xc) % people);
        const auto club = scenario.club(scenario.r32(offers[i] + 8) % 100);
        Writes memory;
        write_date(memory, scratch + 0x4000, year, day + 3, 0);
        for (const auto& terms : {std::array<std::uint32_t, 4>{scratch + 0x4000, 0, 1, 1}, {scratch + 0x4000, 1, 0, 0}, {0, 0, 0, 0}})
            cases.push_back({.entry = 0x103e3170, .registers = with_toc({{3, manager}, {4, person}, {5, club}, {6, terms[0]}, {7, terms[1]}, {8, terms[2]}, {9, terms[3]}}),
                             .memory = memory, .label = label("build offer", unsigned(i), terms[1], terms[2])});
    }

    // ---- 0x103e2db0 and 0x103e3240: real offers and a copy (scratch) with and without a list.
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        const auto at = scratch + 0x5000 + std::uint32_t(i) * 0x100, copy_at = at + 0x80;
        for (const std::uint32_t mark : {0u, 1u})
            for (const std::uint32_t slot : {0u, 1u, 2u})
                for (const std::uint32_t kind : {0u, 1u}) {
                    Writes memory;
                    scratch_offer(scenario, memory, at, offer, {{4, 0x30, slot == 0 ? 0xffffffffu : slot == 1 ? scenario.r32(offer + 0x30) : 3u}, {1, 0x24, kind}});
                    scratch_offer(scenario, memory, copy_at, offer, {{4, 4, slot == 2 ? 0u : scenario.r32(offer + 4)}});
                    if ((i + slot) % 2) append(memory, stale(scenario, 0x80, salt++));
                    Case one{.entry = 0x103e2db0, .registers = with_toc({{3, manager}, {4, at}, {5, copy_at}, {6, mark}}), .memory = memory,
                             .label = label("add approach", unsigned(i), mark, slot, kind)};
                    cases.push_back(one);
                    Case answered = one;
                    answered.answers.push_back({0x102cdbf0, 0});
                    answered.label += " no";
                    cases.push_back(answered);
                    Case yes = one;
                    yes.answers.push_back({0x102cdbf0, 1});
                    yes.label += " yes";
                    cases.push_back(yes);
                }
        Case local{.entry = 0x103e3240, .registers = with_toc({{3, manager}, {4, offer}, {5, 0}, {6, 1}, {7, 1}, {8, 0}}), .label = label("approach from terms", unsigned(i))};
        cases.push_back(local);
        Case other = local;
        other.registers = with_toc({{3, manager}, {4, offer}, {5, 1}, {6, 0}, {7, 2}, {8, 1}});
        other.memory.push_back({4, manager + 0xa78, 0});
        other.label = label("approach from terms no reports", unsigned(i));
        cases.push_back(other);
    }
    if (!offers.empty()) {
        const auto offer = offers[0], at = scratch + 0x6000, copy_at = at + 0x80;
        const auto size = scenario.r32(manager + 0x10), array = scenario.r32(manager + 0xc);
        Writes base;
        scratch_offer(scenario, base, at, offer, {{4, 0x30, 0xffffffffu}});
        scratch_offer(scenario, base, copy_at, offer);
        Writes grown = base;
        grown.push_back({4, manager + 0x14, size});
        cases.push_back({.entry = 0x103e2db0, .registers = with_toc({{3, manager}, {4, at}, {5, copy_at}, {6, 1}}), .memory = grown, .label = "add approach grown"});
        Writes first_table = grown;
        first_table.push_back({4, manager + 0xc, 0});
        cases.push_back({.entry = 0x103e2db0, .registers = with_toc({{3, manager}, {4, at}, {5, copy_at}, {6, 0}}), .memory = first_table, .label = "add approach first table"});
        Writes full = base;
        full.push_back({4, manager + 0x14, size + 1});
        for (std::uint32_t k = 0; k < size && k < 4000; ++k)
            if (!scenario.r32(array + k * 4)) full.push_back({4, array + k * 4, offer});
        cases.push_back({.entry = 0x103e2db0, .registers = with_toc({{3, manager}, {4, at}, {5, copy_at}, {6, 0}}), .memory = full, .label = "add approach no slot"});
    }

    // ---- 0x1040d5c0: real offers, the terms record a real offer too; the second contract lookup failing once.
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto offer = offers[i];
        Case one{.entry = 0x1040d5c0, .registers = with_toc({{3, manager}, {4, offer}, {5, offers[(i + 1) % offers.size()]}}), .label = label("offer news", unsigned(i))};
        cases.push_back(one);
        Case missing = one;
        missing.nth_answers.push_back({0x10082f20, 2, 0});
        missing.label += " no contract";
        cases.push_back(missing);
        Case dirty = one;
        append(dirty.memory, stale(scenario, 0x1f0, salt++));
        dirty.label += " stale";
        cases.push_back(dirty);
    }

    // ---- 0x103f3bf0: club levels answered (the classes of the status), people with contracts spread over the table.
    {
        const std::uint32_t levels[] = {0, 1, 2, 3, 5, 6, 10, 11, 14, 20, 0x1234, 0x10005};
        const auto pool = scenario.toc(-0x4ba0);
        const auto out = scratch_base + 0x7000;
        unsigned n = 0;
        for (std::size_t i = 0; i < offers.size(); ++i)
            for (unsigned index = 0; index < people; index += people / 9 + 1, ++n)
                for (const auto level : levels) {
                    if ((n + level) % 3) continue;
                    const auto at = scratch + 0x7100;
                    Writes memory;
                    scratch_offer(scenario, memory, at, offers[i], {{4, 0xc, index}, {4, 4, n % 2 ? scenario.r32(offers[i] + 4) : 0u}, {1, 0x11, n % 5}});
                    if (n % 4 == 0) append(memory, stale(scenario, 0xc0, salt++));
                    // The comparison constant of the strong branch: small (the shortlist value passes) or huge.
                    write_double(memory, pool + 0xc0, n % 3 == 0 ? 0xfff0000000000000ull : n % 3 == 1 ? 0x7ff0000000000000ull : 0x7ff8000000000000ull);
                    Case one{.entry = 0x103f3bf0, .registers = with_toc({{3, manager}, {4, at}, {5, out}, {6, out + 4}}), .memory = memory,
                             .label = label("first status", unsigned(i), index, level)};
                    one.answers.push_back({level_entry, level});
                    cases.push_back(one);
                }
    }

    // ---- 0x103e3410: people spread over the table and those of real offers, the amounts, caps and years.
    {
        const auto date = scratch_base + 0x8000;
        std::vector<std::uint32_t> persons;
        for (unsigned index = 0; index < people; index += people / 25 + 1) persons.push_back(index);
        for (const auto offer : offers) persons.push_back(scenario.r32(offer + 0xc));
        const std::uint32_t amounts[] = {0xffffffffu, 0, 50000, 0x7fff0000u, 1};
        const std::int32_t caps[] = {-1, 1000, 0x7fffffff, 0};
        const std::uint32_t years_list[] = {0, 3, 0xffffffffu, 200, 1};
        unsigned n = 0;
        for (const auto index : persons) {
            ++n;
            Writes memory;
            write_date(memory, date, year, day + n % 7, 0);
            const auto club = scenario.club(n % 100);
            for (std::size_t a = 0; a < 5; ++a) {
                if ((n + a) % 2) continue;
                const auto cap = caps[(n + a) % 4];
                const auto years = years_list[(n * 3 + a) % 5];
                Writes writes = memory;
                if ((n + a) % 3 == 0) append(writes, stale(scenario, 0x1b0, salt++));
                cases.push_back({.entry = 0x103e3410,
                                 .registers = with_toc({{3, manager}, {4, scenario.person(index % people)}, {5, club}, {6, date}, {7, amounts[a]}, {8, std::uint32_t(cap)}, {9, years}}),
                                 .memory = writes, .label = label("regen offer", index, unsigned(a), unsigned(n))});
            }
        }
    }

    // ---- 0x103e3de0: dates before, at and after today, the club's level, the loan club and the contract answered.
    {
        const auto from_at = scratch_base + 0x8100, to_at = scratch_base + 0x8200;
        unsigned n = 0;
        for (unsigned index = 0; index < people; index += people / 40 + 1) {
            ++n;
            const auto person = scenario.person(index);
            const auto club = scenario.club((n * 7) % 100);
            for (unsigned shape = 0; shape < 24; ++shape) {
                if ((n + shape) % 3 == 0) continue;
                Writes memory;
                const auto from_year = year + (shape % 3) - 1, from_day = day + (shape / 3 % 3) * 5 - 5;
                const auto to_year = year + (shape / 9 % 3) - 1, to_day = day + (shape / 2 % 3) * 5 - 5;
                write_date(memory, from_at, from_year, from_day, 0);
                write_date(memory, to_at, to_year, to_day, 0);
                // The person's recorded date (+0x54) before, at or after the end date.
                write_date(memory, person + 0x54, to_year + (shape % 2) - (shape % 5 == 0), to_day + (shape % 4) - 1, 0);
                Case one{.entry = 0x103e3de0, .registers = with_toc({{3, manager}, {4, person}, {5, club}, {6, from_at}, {7, to_at}}), .memory = memory,
                         .label = label("sign person", index, shape)};
                one.answers.push_back({0x100c9070, shape % 7 == 0 ? 0x32u : 0u});
                if (shape % 4 == 1) one.answers.push_back({0x100ce320, 0});
                if (shape % 6 == 2) one.answers.push_back({0x10083110, 0});
                if (shape % 5 == 3) one.answers.push_back({0x103ecef0, 0});
                if (shape % 8 == 4) one.answers.push_back({0x104ae570, 0});
                // The contract manager's 0x10083e50 reads data files a snapshot has no host for: answered
                // in most cases so the calls after it are compared.
                if (shape % 5 != 0) one.answers.push_back({0x10083e50, 0});
                cases.push_back(one);
            }
        }
    }
}
}
