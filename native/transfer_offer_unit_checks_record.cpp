// TRANSFER_OFFER.CPP checker cases for the offer record (transfer_offer_unit_record.hpp).
//
// An offer is 0x54 bytes of plain values plus two list pointers. The career snapshot's transfer manager
// (global -0x7b40) holds 21 live offers, every one with null lists, so the cases take real offers, people,
// clubs and the manager's own list objects (the 16-byte lists at manager+0x18) where the routine only needs
// such a thing, and build in the scratch area (a) offer records and (b) lists of rows in the game's list
// layout (header: halfword kind at +0, head node +4, tail +8, cursor +0xc; node: row, ?, next) where a
// routine reads rows whose content matters (people indices) and never frees the list.
//
// The two file routines are answered on both sides (Case::answers has one answer for all calls of a
// routine): all calls of the file routine report one item, or none, so of each file routine's failure
// sites only the first can be reached. Case::pokes deliver what a read would read into the frame buffers.
#include "transfer_offer_unit_checks.hpp"
#include "guest_abi.hpp"
#include <set>
#include <string>
#include <tuple>

namespace cm::aot::transfer_offer_unit_checks {
namespace {
constexpr std::uint32_t construct = 0x10411e60, construct_with = 0x10411ea0, destroy = 0x10411f10,
                        read_file = 0x10411fa0, write_file = 0x104123e0, start_plain = 0x104163d0,
                        start_amount = 0x104164f0, total = 0x10416710, deferred_amount = 0x10416d30,
                        status_of = 0x10416d60, exchange_row_of = 0x10417260;
constexpr std::uint32_t file_read = 0x10747a80, file_write = 0x10747cc0;
constexpr std::uint32_t offer_area = scratch_base + 0x1000, key_area = scratch_base + 0x1100,
                        exchange_area = scratch_base + 0x2000,
                        table_area = scratch_base + 0x4000, fake_file = 0x1234;

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
};

// An offer in the scratch area: every byte set, so no case depends on what the area held.
Memory blank_offer(std::uint32_t fill) {
    Memory memory;
    for (std::uint32_t at = 0; at < 0x54; at += 4) memory.put(offer_area + at, fill);
    return memory;
}

// A list of the given row pointers in the game's list layout, at `at` (header) with nodes after it.
std::uint32_t build_list(Memory& memory, std::uint32_t at, std::uint16_t kind, const std::vector<std::uint32_t>& rows) {
    const auto node = [&](std::size_t n) { return at + 0x20 + std::uint32_t(n) * 0x10; };
    memory.put(at, kind << 16 | std::uint32_t(rows.size()));
    memory.put(at + 4, rows.empty() ? 0 : node(0));
    memory.put(at + 8, rows.empty() ? 0 : node(rows.size() - 1));
    memory.put(at + 0xc, 0);
    for (std::size_t n = 0; n < rows.size(); ++n) {
        memory.put(node(n), rows[n]);
        memory.put(node(n) + 4, 0);
        memory.put(node(n) + 8, n + 1 < rows.size() ? node(n + 1) : 0);
    }
    return at;
}
// An exchange row (person index, status byte) at `at`.
std::uint32_t exchange_row(Memory& memory, std::uint32_t at, std::uint32_t person, std::uint32_t status) {
    memory.put(at, person);
    memory.put(at + 4, status << 24);
    return at;
}
// A list of exchange rows for the given (person, status) pairs.
std::uint32_t exchange_list(Memory& memory, const std::vector<std::pair<std::uint32_t, std::uint32_t>>& rows) {
    std::vector<std::uint32_t> pointers;
    for (std::size_t n = 0; n < rows.size(); ++n)
        pointers.push_back(exchange_row(memory, exchange_area + 0x400 + std::uint32_t(n) * 8, rows[n].first, rows[n].second));
    return build_list(memory, exchange_area, 2, pointers);
}
void append(Memory& to, const Memory& from) { to.writes.insert(to.writes.end(), from.writes.begin(), from.writes.end()); }
}

TRANSFER_OFFER_UNIT_CASES("record") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7b40);
    const auto slots = scenario.r32(manager), limit = scenario.r32(manager + 4);
    std::vector<std::uint32_t> offers;  // the live offers
    for (std::uint32_t i = 0; i < limit; ++i)
        if (const auto offer = scenario.r32(slots + i * 4)) offers.push_back(offer);
    if (offers.size() < 3) return;
    // The manager's own lists that have rows: a kind-1 one and one of another kind when there is one.
    std::uint32_t list_one = 0, list_other = 0;
    for (std::uint32_t g = 0; g < 50; ++g)
        for (std::uint32_t l = 0; l < 3; ++l) {
            const auto list = manager + 0x18 + g * 0x30 + l * 16;
            if (scenario.r32(list + 4) == 0) continue;
            if ((scenario.r32(list) >> 16) == 1) { if (!list_one) list_one = list; }
            else if (!list_other) list_other = list;
        }
    const auto stack = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    const auto person_of = [&](std::size_t n) { return scenario.r32(offers[n % offers.size()] + 0x0c); };
    const std::vector<std::uint32_t> people = {person_of(0), person_of(1), person_of(2), 5, 40};

    // 0x10411e60
    {
        auto memory = blank_offer(0xa5a5a5a5);
        cases.push_back({.entry = construct, .registers = {{3, offer_area}}, .memory = memory.writes, .label = "construct"});
    }

    // 0x10411ea0: an empty record, then the fill by 0x104126d0 (another block) from a person, a club, a null
    // or a record argument, an amount and a flag.
    for (const auto person : {people[0], people[1], 0u})
        for (const std::uint32_t club : {scenario.club(2), scenario.club(77), 0u}) {
            if (person == 0 && club == 0) continue;
            for (const std::uint32_t amount : {100000u, 7500000u, 0u}) {
                auto memory = blank_offer(0x5a5a5a5a);
                cases.push_back({.entry = construct_with,
                                 .registers = {{3, offer_area}, {4, person ? scenario.person(person) : 0}, {5, club}, {6, 0},
                                               {7, amount}, {8, amount > 100000}},
                                 .memory = memory.writes,
                                 .label = "construct_with person " + std::to_string(person) + " amount " + std::to_string(amount)});
            }
        }

    // 0x10411f10: the record's lists released, the record kept or freed. 0x10738c20 (the destroy of a kind-1
    // list and of the exchange list) is run in place by its owner in the original, which a case's answers
    // never reach, and it needs a heap list the snapshot does not have for a record (all 21 live offers have
    // null lists): those calls are only reached by real calls. A club list of another kind goes to 0x10738e60
    // and the freeing to 0x107735a0; both are answered, so what is compared is the calls and their arguments.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> releases = {{0x10738e60, 0}, {0x107735a0, 0}};
    for (const std::uint16_t clubs : {0, 2, 0x101, 0xffff, 0})
        for (const std::uint32_t deleting : {0u, 1u, 0xffffu, 0x18000u, 0x10001u, 0x7fffu}) {
            Memory memory;
            append(memory, blank_offer(0));
            if (clubs) memory.put(offer_area, build_list(memory, scratch_base + 0x5000, clubs, {}));
            cases.push_back({.entry = destroy, .registers = {{3, offer_area}, {4, deleting}}, .memory = memory.writes,
                             .label = "destroy club list kind " + std::to_string(clubs) + " deleting " + std::to_string(deleting),
                             .answers = releases});
        }
    cases.push_back({.entry = destroy, .registers = {{3, 0}, {4, 1}}, .label = "destroy null", .answers = releases});
    cases.push_back({.entry = destroy, .registers = {{3, offers[0]}, {4, 1}}, .label = "destroy live offer, freed"});
    cases.push_back({.entry = destroy, .registers = {{3, offers[1]}, {4, 0}}, .label = "destroy live offer, kept"});

    // 0x10411fa0: the read. The file layer is answered (one item each time); the pokes hold what the
    // reads deliver. The table is the manager's offer slots (the real call's), whose entries' first word
    // is a club list pointer (null in all live offers).
    {
        struct Read { const char* label; std::uint32_t answer, count, index, bound, table; bool shared_list; bool null_entry; };
        const std::vector<Read> reads = {
            {"read, no lists", 1, 0, 0, 0, slots, false, false},
            {"read, one count of two", 1, 2, 3, 21, slots, false, false},
            {"read, shared club list", 1, 2, 3, 21, slots, true, false},
            {"read, index beyond the bound", 1, 3, 30, 21, slots, false, false},
            {"read, negative index", 1, 2, 0xffffffff, 21, slots, false, false},
            {"read, missing table", 1, 2, 3, 21, 0, false, false},
            {"read, entry missing", 1, 2, 1, 21, table_area, false, true},
            {"read, negative count", 1, 0xffff, 3, 21, slots, false, false},
            {"read, long lists", 1, 40, 3, 21, slots, false, false},
            {"read, no record", 0, 2, 3, 21, slots, false, false},
        };
        for (const auto& item : reads) {
            auto memory = blank_offer(0x11112222);
            memory.put(offer_area + 0x28, item.bound);
            if (item.null_entry) {
                memory.put(table_area, 0);
                memory.put(table_area + 4, 0);
            }
            if (item.shared_list) memory.put(offers[3] + 0, list_one);
            const auto buffer = stack - 0x180;
            Memory poke;
            poke.put(buffer + 0x38, item.count, 2);
            poke.put(buffer + 0x3c, item.index);
            poke.put(buffer + 0x48, 77);
            poke.put(buffer + 0x4c, 2u << 24);
            cases.push_back({.entry = read_file, .registers = {{3, offer_area}, {4, fake_file}, {5, item.table}},
                             .memory = memory.writes, .label = item.label,
                             .answers = {{file_read, item.answer}}, .pokes = {{file_read, poke.writes}}});
        }
    }

    // The same reads with one of them short: the nth read of a run (1 record, 2 club count, 3-4 club
    // indices, 5 exchange count, 6-7 exchange rows) reports 0 items or 2, which reaches each failure site.
    for (const std::uint32_t nth : {1u, 2u, 3u, 4u, 5u, 6u, 7u})
        for (const std::uint32_t short_count : {0u, 2u}) {
            auto memory = blank_offer(0x11112222);
            memory.put(offer_area + 0x28, 21);
            const auto buffer = stack - 0x180;
            Memory poke;
            poke.put(buffer + 0x38, 2, 2);
            poke.put(buffer + 0x3c, 3);
            poke.put(buffer + 0x48, 77);
            poke.put(buffer + 0x4c, 2u << 24);
            cases.push_back({.entry = read_file, .registers = {{3, offer_area}, {4, fake_file}, {5, slots}},
                             .memory = memory.writes,
                             .label = "read, read " + std::to_string(nth) + " short " + std::to_string(short_count),
                             .answers = {{file_read, 1}}, .pokes = {{file_read, poke.writes}},
                             .nth_answers = {{file_read, nth, short_count}}});
        }
    // The writer with its nth write short: 1 record, 2 club count, 3 club row, 4 exchange count, 5 exchange row
    // (the manager's lists have one row each).
    for (const std::uint32_t nth : {1u, 2u, 3u, 4u, 5u, 6u})
        for (const std::uint32_t short_count : {0u, 2u}) {
            auto memory = blank_offer(0x33);
            memory.put(offer_area, list_one);
            memory.put(offer_area + 4, list_one);
            cases.push_back({.entry = write_file, .registers = {{3, offer_area}, {4, fake_file}}, .memory = memory.writes,
                             .label = "write, write " + std::to_string(nth) + " short " + std::to_string(short_count),
                             .answers = {{file_write, 1}}, .nth_answers = {{file_write, nth, short_count}}});
        }
    // 0x104123e0: the write, of records with no lists, the manager's lists and every combination.
    for (const auto offer : {offers[0], offers[1], offers[2]})
        cases.push_back({.entry = write_file, .registers = {{3, offer}, {4, fake_file}}, .label = "write live offer",
                         .answers = {{file_write, 1}}});
    for (const auto clubs : {0u, list_one, list_other})
        for (const auto exchange : {0u, list_one, list_other})
            for (const std::uint32_t answer : {1u, 0u, 2u}) {
                auto memory = blank_offer(0x33);
                memory.put(offer_area, clubs);
                memory.put(offer_area + 4, exchange);
                cases.push_back({.entry = write_file, .registers = {{3, offer_area}, {4, fake_file}}, .memory = memory.writes,
                                 .label = "write lists " + std::to_string(clubs != 0) + std::to_string(clubs == list_other) +
                                          std::to_string(exchange != 0) + std::to_string(exchange == list_other) +
                                          " answer " + std::to_string(answer),
                                 .answers = {{file_write, answer}}});
            }

    // 0x104163d0 and 0x104164f0: starting an offer. Every byte of the area is set first, so the bytes the
    // routines leave show.
    for (const auto person : {people[0], people[2]})
        for (const std::uint32_t club : {scenario.club(3), scenario.club(90)}) {
            auto memory = blank_offer(0xa5a5a5a5);
            cases.push_back({.entry = start_plain, .registers = {{3, offer_area}, {4, scenario.person(person)}, {5, club}},
                             .memory = memory.writes, .label = "start plain " + std::to_string(person)});
        }
    for (const auto& [person, club] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{scenario.person(5), 0}, {0, scenario.club(3)}, {0, 0}}) {
        auto memory = blank_offer(0xa5a5a5a5);
        cases.push_back({.entry = start_plain, .registers = {{3, offer_area}, {4, person}, {5, club}}, .memory = memory.writes,
                         .label = "start plain, a record missing"});
        cases.push_back({.entry = start_amount, .registers = {{3, offer_area}, {4, person}, {5, club}, {6, 12345}},
                         .memory = memory.writes, .label = "start with amount, a record missing"});
    }
    {
        // One club per distinct label: the person's own club matches one of them. The sample table's
        // base is set so the draw is a multiple of 20 (or not), the cursor at the table's end makes
        // the draw take a new base from the generator.
        const auto cursor_cell = scenario.toc(-0x7b84), base_cell = scenario.toc(-0x7b90), last = scenario.global(-0x7b88);
        const auto cursor = scenario.r32(cursor_cell);
        const auto next = cursor + 2;
        const auto sample = std::uint32_t(scenario.s.memory.read<2>(next));
        std::vector<std::uint32_t> labelled;  // first club of each distinct label
        {
            std::set<std::uint32_t> seen;
            for (std::uint32_t k = 0; k < 600; ++k) {
                const auto club = scenario.club(k);
                if (scenario.r32(club) != k) break;
                if (seen.insert(scenario.r32(club + 0x30)).second) labelled.push_back(club);
            }
        }
        for (const auto person : {people[0], people[1], people[2]}) {
            const auto record = scenario.person(person);
            const auto attribute = scenario.r32(record + 0x14) + 10;
            for (const auto club : labelled)
                for (const auto& [multiple, level, wrap] : std::vector<std::tuple<bool, std::uint32_t, bool>>{
                         {true, 7001, false}, {false, 7001, false}, {true, 7000, false}, {true, 7001, true}}) {
                    auto memory = blank_offer(0xa5a5a5a5);
                    memory.put(base_cell, (20 - sample % 20) % 20 + (multiple ? 0 : 1));
                    memory.put(attribute, level, 2);
                    if (wrap) memory.put(cursor_cell, last);
                    cases.push_back({.entry = start_amount,
                                     .registers = {{3, offer_area}, {4, record}, {5, club}, {6, 250000}},
                                     .memory = memory.writes,
                                     .label = "start with amount person " + std::to_string(person) + " label club " +
                                              std::to_string(scenario.r32(club)) + (multiple ? " multiple" : " odd") + " level " +
                                              std::to_string(level) + (wrap ? " wrap" : "")});
                }
        }
    }

    // 0x10416710: the total, over a built offer with every term and the live offers.
    for (const std::int32_t percent : {0, 5, 100, 127, -5})
        for (const std::uint32_t exchange : {0u, 1u, 3u}) {
            Memory memory;
            std::vector<std::pair<std::uint32_t, std::uint32_t>> rows;
            for (std::uint32_t n = 0; n < exchange; ++n) rows.push_back({people[n] + n, 1});
            auto list = blank_offer(0);
            const auto head = exchange ? exchange_list(memory, rows) : 0u;
            append(memory, list);
            memory.put(offer_area + 4, head);
            memory.put(offer_area + 0x0c, people[1]);
            memory.put(offer_area + 0x20, 1000000);
            memory.put(offer_area + 0x40, 250000);
            memory.put(offer_area + 0x48, 70000);
            memory.put(offer_area + 0x50, std::uint32_t(percent), 1);
            for (const std::uint32_t flags : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 15u}) {
                cases.push_back({.entry = total,
                                 .registers = {{3, offer_area}, {4, flags & 1 ? 0x100u : 0u}, {5, flags & 2 ? 1u : 0u},
                                               {6, flags & 4 ? 0x180u : 0u}, {7, flags & 8 ? 0x100u : 0x81u}},
                                 .memory = memory.writes,
                                 .label = "total sell-on " + std::to_string(percent) + " exchange " + std::to_string(exchange) +
                                          " flags " + std::to_string(flags)});
                cases.push_back({.entry = total,
                                 .registers = {{3, offer_area}, {4, flags & 1 ? 1u : 0u}, {5, flags & 2 ? 0x100u : 0u},
                                               {6, flags & 4 ? 1u : 0u}, {7, flags & 8 ? 0u : 0x1ffu}},
                                 .memory = memory.writes,
                                 .label = "total (low bytes) sell-on " + std::to_string(percent) + " exchange " + std::to_string(exchange) +
                                          " flags " + std::to_string(flags)});
            }
        }
    for (const auto offer : offers)
        for (const std::uint32_t flags : {0u, 1u, 2u, 4u, 8u})
            cases.push_back({.entry = total,
                             .registers = {{3, offer}, {4, flags & 1}, {5, flags & 2}, {6, flags & 4}, {7, flags & 8 ? 0u : 1u}},
                             .label = "total live offer flags " + std::to_string(flags)});
    for (const std::int32_t instalments : {-5, 0, 1, 250000})
        for (const std::int32_t extra : {-1, 0, 7, 0x7fffffff}) {
            auto memory = blank_offer(0);
            memory.put(offer_area + 0x0c, people[0]);
            memory.put(offer_area + 0x20, 0x7ffffff0);
            memory.put(offer_area + 0x40, std::uint32_t(instalments));
            memory.put(offer_area + 0x48, std::uint32_t(extra));
            memory.put(offer_area + 0x50, 50, 1);
            cases.push_back({.entry = total, .registers = {{3, offer_area}, {4, 0}, {5, 1}, {6, 0}, {7, 1}}, .memory = memory.writes,
                             .label = "total wrapping " + std::to_string(instalments) + " " + std::to_string(extra)});
        }

    // 0x10416d30
    for (const std::int32_t instalments : {-5, 0, 1, 250000})
        for (const std::int32_t extra : {-1, 0, 7, 0x7fffffff}) {
            auto memory = blank_offer(0);
            memory.put(offer_area + 0x40, std::uint32_t(instalments));
            memory.put(offer_area + 0x48, std::uint32_t(extra));
            cases.push_back({.entry = deferred_amount, .registers = {{3, offer_area}}, .memory = memory.writes,
                             .label = "deferred " + std::to_string(instalments) + " " + std::to_string(extra)});
        }
    for (const auto offer : offers)
        cases.push_back({.entry = deferred_amount, .registers = {{3, offer}}, .label = "deferred live offer"});

    // 0x10416d60 and 0x10417260 over the offer's own player, an exchange list and a live offer.
    for (const std::uint32_t kind : {0u, 3u, 0x83u})
        for (const std::uint32_t asked : {77u, 78u, 90u, 91u, 92u})
            for (const bool with_list : {false, true}) {
                Memory memory;
                std::uint32_t list = 0;
                if (with_list) list = exchange_list(memory, {{90, 1}, {91, 2}, {92, 0x83}});
                append(memory, blank_offer(0));
                memory.put(offer_area + 0x04, list);
                memory.put(offer_area + 0x0c, 77);
                memory.put(offer_area + 0x10, 2, 1);
                memory.put(offer_area + 0x34, kind, 1);
                cases.push_back({.entry = status_of, .registers = {{3, offer_area}, {4, asked}}, .memory = memory.writes,
                                 .label = "status kind " + std::to_string(kind) + " person " + std::to_string(asked) +
                                          (with_list ? " with list" : "")});
            }
    for (const auto offer : offers)
        cases.push_back({.entry = status_of, .registers = {{3, offer}, {4, scenario.r32(offer + 0x0c)}}, .label = "status live offer"});
    for (const std::uint32_t key : {90u, 91u, 92u, 93u}) {
        Memory memory;
        const auto list = exchange_list(memory, {{90, 1}, {91, 2}, {92, 0x83}});
        append(memory, blank_offer(0));
        memory.put(offer_area + 0x04, list);
        memory.put(key_area, key);
        cases.push_back({.entry = exchange_row_of, .registers = {{3, offer_area}, {4, key_area}}, .memory = memory.writes,
                         .label = "row of " + std::to_string(key)});
    }
    {
        auto memory = blank_offer(0);
        memory.put(key_area, 77);
        cases.push_back({.entry = exchange_row_of, .registers = {{3, offer_area}, {4, key_area}}, .memory = memory.writes,
                         .label = "row of, no exchange list"});
    }
}
}
