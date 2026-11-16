#include "transfer_manager_native_status.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t status_rows = 0x9a0, status_row_size = 10, status_row_term = 8;
constexpr std::uint32_t person_term = 0x5e, contract_term = 0x42;
constexpr std::uint32_t find_contract = 0x10082f20;  // (contract manager, person) -> contract or 0
constexpr std::uint32_t bsearch_rows = 0x10775600;   // (key, rows, count, 0x10, compare context) -> row or 0
constexpr std::uint32_t finance_account = 0x10103510, add_income = 0x100f7050, add_expense = 0x100f70e0;
constexpr std::uint32_t note_fee = 0x100132f0, next_listed = 0x10738e00;

std::uint8_t status_flags(Cpu& c, std::uint32_t manager, std::uint32_t index) {
    return c.u8(c.u32(manager + status_rows) + index * status_row_size);
}
// The club of the person's contract (0 without one); the lookup leaves its result in the shared found-contract cell.
std::uint32_t club_of_person(Cpu& c, std::uint32_t contracts, std::uint32_t found_cell, std::uint32_t clubs_cell,
                             std::uint32_t person, std::uint32_t back) {
    const auto contract = invoke(c, find_contract, back, {c.u32(contracts), person});
    c.put32(found_cell, contract);
    return contract ? c.u32(clubs_cell) + c.u32(contract + 4) * club_size : 0;
}
}

std::int32_t remaining_status_term(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int8_t status) {
    Frame frame(c, 0x103e9220, 0x50, {.first_gpr = 29});
    const auto contracts = c.toc(toc::contracts);
    keep(c, 31, contracts);
    keep(c, 30, 0);
    keep(c, 29, person);
    if (status == 4) {
        const auto row = c.u32(manager + status_rows) + c.u32(person) * status_row_size;
        return std::int32_t(c.u8(person + person_term)) - std::int32_t(c.u8(row + status_row_term));
    }
    // The first lookup passes the person in r4 too: the original leaves its argument there.
    if (!invoke(c, find_contract, 0x103e9278, {c.u32(contracts), person})) return 0;
    const auto contract = invoke(c, find_contract, 0x103e928c, {c.u32(contracts), person});
    return c.i16(contract + contract_term);
}

void start_offer_with_weekday(Cpu& c, std::uint32_t offer_owner, std::uint32_t record) {
    Frame frame(c, 0x103ed090, 0x50, {.first_gpr = 28});
    const HalfwordDraw draw(c);
    keep(c, 31, draw.cursor);
    keep(c, 30, draw.seed);
    keep(c, 29, record);
    keep(c, 28, offer_owner);
    invoke_native(c, 0x1040ea40, 0x103ed0c0, {offer_owner, record});
    c.put8(record + 0x34, 10);
    draw.advance(0x103ed0f0);
    const auto drawn = std::int32_t(draw.value());
    const auto weekday = std::int8_t(drawn % 7 + 18);
    invoke(c, 0x103eb4a0, 0x103ed144, {offer_owner, record, std::uint32_t(std::int32_t(weekday)), 0});
}

void settle_offer_statuses(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    constexpr std::uint32_t flag_fee = 8, flag_second = 2, flag_third = 4, offer_person = 0xc, offer_list = 4,
                            list_cursor = 0xc, list_start = 4;
    Frame frame(c, 0x103ef8a0, 0x50, {.first_gpr = 27});
    const auto people = c.toc(toc::people);
    keep(c, 29, offer);
    keep(c, 31, people);
    keep(c, 28, manager);
    const auto person_at = [&](std::uint32_t index) { return c.u32(people) + index * person_size; };
    const auto first = person_at(c.u32(offer + offer_person));
    keep(c, 30, first);
    // The offer's own person: the flags are read again at every test, as the original does.
    const auto own_flags = [&] { return status_flags(c, manager, c.u32(offer + offer_person)); };
    if (own_flags() & flag_fee) {
        invoke_native(c, 0x103f2560, 0x103ef8ec, {manager, first, offer});
        invoke(c, 0x103f29f0, 0x103ef8f8, {manager, first});
    }
    if (own_flags() & flag_second) {
        invoke_native(c, 0x103f2800, 0x103ef930, {manager, person_at(c.u32(offer + offer_person)), 0xffffffff, 2, offer});
        invoke(c, 0x103f2aa0, 0x103ef93c, {manager, first});
    }
    if (own_flags() & flag_third) {
        invoke_native(c, 0x103f2800, 0x103ef974, {manager, person_at(c.u32(offer + offer_person)), 0xffffffff, 5, offer});
        invoke(c, 0x103f2b50, 0x103ef980, {manager, first});
    }
    const auto list = c.u32(offer + offer_list);
    if (!list) return;
    c.put32(list + list_cursor, c.u32(list + list_start));
    // The listed people: the fee flag's follow-up is still the offer's own person (0x103ef9d0 passes r30).
    for (;;) {
        const auto entry = invoke(c, next_listed, 0x103efa7c, {c.u32(offer + offer_list)});
        keep(c, 27, entry);
        if (!entry) return;
        const auto flags = [&] { return status_flags(c, manager, c.u32(entry)); };
        if (flags() & flag_fee) {
            invoke_native(c, 0x103f2560, 0x103ef9c8, {manager, person_at(c.u32(entry)), offer});
            invoke(c, 0x103f29f0, 0x103ef9d4, {manager, first});
        }
        if (flags() & flag_second) {
            invoke_native(c, 0x103f2800, 0x103efa0c, {manager, person_at(c.u32(entry)), 0xffffffff, 2, offer});
            invoke(c, 0x103f2aa0, 0x103efa24, {manager, person_at(c.u32(entry))});
        }
        if (flags() & flag_third) {
            invoke_native(c, 0x103f2800, 0x103efa5c, {manager, person_at(c.u32(entry)), 0xffffffff, 5, offer});
            invoke(c, 0x103f2b50, 0x103efa74, {manager, person_at(c.u32(entry))});
        }
    }
}

std::uint32_t settle_status_row_fee(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int32_t target,
                                    std::uint32_t kind, std::uint32_t extra) {
    constexpr std::uint32_t kind_second = 5, key_at = 0x38;
    Frame frame(c, 0x103f2800, 0x70, {.first_gpr = 22});
    const auto contracts = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    const auto compare_context = c.toc(-0x4bb4), finance_cell = c.toc(-0x7ab8);
    keep(c, 22, manager);
    keep(c, 27, compare_context);
    keep(c, 23, person);
    keep(c, 28, finance_cell);
    keep(c, 24, std::uint32_t(target));
    keep(c, 29, contracts);
    keep(c, 25, kind);
    keep(c, 30, found_cell);
    keep(c, 26, extra);
    keep(c, 31, clubs_cell);
    // The first lookup is made with the person in r4 as the caller left it.
    if (!club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f2840)) return 0;
    c.put32(frame.at(key_at), c.u32(person));
    const bool second = std::int8_t(kind) == std::int8_t(kind_second);
    const auto row = invoke(c, bsearch_rows, second ? 0x103f28a0 : 0x103f28bc,
                            {frame.at(key_at), c.u32(manager + (second ? 0x9cc : 0x9c0)),
                             c.u32(manager + (second ? 0x9d4 : 0x9c8)), 0x10, compare_context});
    keep(c, 27, row);
    if (!row) return 0;
    if (std::int32_t(c.i8(row + 8)) != target && target != -1) return 0;
    if (target <= 0) return 0;
    invoke_native(c, 0x10406e00, 0x103f2900, {manager, row, std::uint32_t(target), kind, extra});
    const auto fee_club = c.u32(clubs_cell) + c.u32(row + 0xc) * club_size;
    const auto income = invoke(c, finance_account, 0x103f2918, {c.u32(finance_cell), fee_club});
    keep(c, 26, income);
    const auto person_club = club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f292c);
    const auto expense = invoke(c, finance_account, 0x103f2958, {c.u32(finance_cell), person_club});
    keep(c, 22, expense);
    const auto club_again = club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f296c);
    const auto fee_club_again = c.u32(clubs_cell) + c.u32(row + 0xc) * club_size;
    invoke(c, note_fee, 0x103f29b0, {c.u32(c.toc(-0x7afc)), c.u32(row + 4), club_again, fee_club_again});
    const auto fee = c.u32(row + 4);
    invoke(c, add_income, 0x103f29c0, {income, std::uint32_t(std::int32_t(fee) >> 31), fee});
    invoke(c, add_expense, 0x103f29d0, {expense, std::uint32_t(std::int32_t(c.u32(row + 4)) >> 31), c.u32(row + 4)});
    return 1;
}

void settle_status_fee(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t offer) {
    constexpr std::uint32_t key_at = 0x38, offer_person = 0xc, offer_list = 4, offer_amount = 0x20;
    Frame frame(c, 0x103f2560, 0x70, {.first_gpr = 22});
    const auto contracts = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs);
    const auto finance_cell = c.toc(-0x7ab8), people_cell = c.toc(toc::people);
    keep(c, 29, contracts);
    keep(c, 22, manager);
    keep(c, 27, finance_cell);
    keep(c, 23, person);
    keep(c, 28, people_cell);
    keep(c, 24, offer);
    keep(c, 30, found_cell);
    keep(c, 31, clubs_cell);
    if (!club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f2598)) return;
    // The key's first word stays what the frame held; the second is the person's club id, the fourth the person's
    // id (the row compare tests both).
    c.put32(frame.at(key_at + 12), c.u32(person));
    const auto lookup_club = club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f25d8);
    c.put32(frame.at(key_at + 4), c.u32(lookup_club));
    const auto row = invoke(c, bsearch_rows, 0x103f261c,
                            {frame.at(key_at), c.u32(manager + 0x9b4), c.u32(manager + 0x9bc), 0x10, c.toc(-0x4bb0)});
    keep(c, 26, row);
    if (!row) return;
    std::uint32_t amount;
    std::uint32_t own_person;
    if (c.i32(person) == c.i32(offer + offer_person)) {
        amount = c.u32(offer + offer_amount);
        keep(c, 25, amount);
        const auto list = c.u32(offer + offer_list);
        if (list) {
            c.put32(list + 0xc, c.u32(list + 4));
            for (;;) {
                const auto entry = invoke(c, next_listed, 0x103f2674, {c.u32(offer + offer_list)});
                if (!entry) break;
                amount += c.u32(c.u32(people_cell) + c.u32(entry) * person_size + 8);
                keep(c, 25, amount);
            }
        }
        own_person = 1;
    } else {
        const auto club = club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f2690);
        amount = invoke(c, 0x10084be0, 0x103f26cc, {c.u32(contracts), person, club, 0, 0xffffffff, 0});
        keep(c, 25, amount);
        own_person = 0;
    }
    const auto share = std::int32_t(amount * std::uint32_t(std::int32_t(c.i8(row)))) / 100;
    keep(c, 28, std::uint32_t(share));
    if (share < 1000) return;
    invoke_native(c, 0x10406910, 0x103f2714, {manager, row, std::uint32_t(share), offer, own_person});
    const auto income = invoke(c, finance_account, 0x103f272c,
                               {c.u32(finance_cell), c.u32(clubs_cell) + c.u32(row + 8) * club_size});
    keep(c, 25, income);
    const auto person_club = club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f2740);
    const auto expense = invoke(c, finance_account, 0x103f276c, {c.u32(finance_cell), person_club});
    keep(c, 24, expense);
    const auto club_again = club_of_person(c, contracts, found_cell, clubs_cell, person, 0x103f2780);
    invoke(c, note_fee, 0x103f27c4,
           {c.u32(c.toc(-0x7afc)), std::uint32_t(share), club_again, c.u32(clubs_cell) + c.u32(row + 8) * club_size});
    const auto sign = std::uint32_t(share >> 31);
    keep(c, 22, sign);
    invoke(c, add_income, 0x103f27d8, {income, sign, std::uint32_t(share)});
    invoke(c, add_expense, 0x103f27e8, {expense, sign, std::uint32_t(share)});
}
}
