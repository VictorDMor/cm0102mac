#include "holiday_transfers.hpp"
#include "transfer_manager_native_recipients.hpp"
#include "transfer_manager_native_recipients_held.hpp"
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;  // (contract manager, person) -> contract or 0
constexpr std::uint32_t history_rows = 0x994, history_capacity = 0x998, history_count = 0x99c, history_row_size = 0x28;
constexpr std::uint32_t manager_shortlist = 0x988;
constexpr std::uint32_t person_name_words = 0x28;  // +0x28, +0x2c, +0x30: records whose word +0x20 names the person
constexpr std::uint32_t kind_loan = 5, kind_transfer = 3;
}

void record_completed_transfer(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t buyer, std::uint32_t signing,
                               std::uint32_t fee, std::uint32_t loan) {
    constexpr std::uint32_t person_profile_link = 0x14, person_club_link = 0x24, person_term = 0x8;
    Frame frame(c, 0x103e68c0, 0xa0, {.first_gpr = 17});
    RecipientsHeld r(c);
    r[17] = c.toc(-0x79e0);
    r[18] = c.toc(-0x79dc);
    r[19] = c.toc(-0x7ba8);
    r[20] = c.toc(-0x4b9c);
    r[21] = manager;
    r[22] = person;
    r[23] = buyer;
    r[24] = signing;
    r[25] = fee;
    r[26] = loan;
    r[27] = c.toc(-0x7a6c);
    r[28] = c.toc(-0x7b80);
    r[29] = c.toc(toc::contracts);
    r[30] = c.toc(toc::found_contract);
    r[31] = c.toc(toc::clubs);
    const auto strings = r[20];
    const auto call = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        r.publish(c);
        return invoke(c, entry, back, arguments);
    };
    const auto call_native = [&](std::uint32_t entry, std::uint32_t back, std::initializer_list<std::uint32_t> arguments) {
        r.publish(c);
        return invoke_native(c, entry, back, arguments);
    };
    const auto contracts = [&] { return c.u32(r[29]); };
    // The club of the person's contract (0 without one); the lookup's result stays in the shared found cell.
    const auto club_of_person = [&](std::uint32_t back) {
        const auto contract = call(find_contract, back, {contracts(), person});
        c.put32(r[30], contract);
        return contract ? c.u32(r[31]) + c.u32(contract + 4) * club_size : 0;
    };

    if (call(0x10083110, 0x103e6914, {contracts()})) return;
    if (c.u32(manager + history_capacity) == c.u32(manager + history_count)) {
        // The history is full: 100 more rows.
        c.put32(manager + history_capacity, c.u32(manager + history_capacity) + 100);
        if (c.u32(manager + history_rows) == 0)
            c.put32(manager + history_rows, call(0x10775130, 0x103e694c, {c.u32(manager + history_capacity) * history_row_size}));
        else
            c.put32(manager + history_rows, call(0x107751b0, 0x103e6960,
                                                 {c.u32(manager + history_rows), c.u32(manager + history_capacity) * history_row_size}));
        if (c.u32(manager + history_rows) == 0) {
            // Out of memory: the report text goes to the message dialog; the success flag is not cleared.
            const auto buffer = r[19];
            call(0x10739890, 0x103e6988, {strings + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
            const auto version = call(0x10768810, 0x103e698c, {});
            call(0x1077b720, 0x103e69ac, {buffer, strings + 0x4c, version, frame.at(0x40), frame.at(0x38), 0xce9, strings + 0x17f7});
            call(0x10727890, 0x103e69b8, {r[18], strings + 0xa6});
            call(0x10727890, 0x103e69c8, {r[17], strings + 0xbe, buffer});
            call(0x10769b60, 0x103e69d8, {r[18], r[17], 0});
            return;
        }
    }
    const auto index = c.u32(manager + history_count);
    const auto row = c.u32(manager + history_rows) + index * history_row_size;
    c.put32(manager + history_count, index + 1);
    c.put32(row + 0x10, 0xffffffff);
    c.put32(row + 0x14, buyer ? c.u32(buyer) : 0xffffffff);
    const bool loaned = std::int8_t(loan) != 0;
    r[17] = r[28] + 2;
    r[18] = r[28] + 4;
    const auto today = r[28];
    c.put16(row + 0x22, c.u16(today + 2));
    c.put16(row + 0x20, c.u16(today));
    c.put8(row + 0x24, c.u8(today + 4));
    c.put32(row, c.u32(person));
    c.put32(row + 4, c.u32(c.u32(person + person_name_words) + 0x20));
    c.put32(row + 8, c.u32(c.u32(person + person_name_words + 4) + 0x20));
    c.put32(row + 0xc, c.u32(c.u32(person + person_name_words + 8) + 0x20));
    c.put8(row + 0x1c, loaned ? kind_loan : kind_transfer);
    c.put32(row + 0x18, 0);
    holiday_transfers::recorded(c, row);
    if (loaned) return;

    if (c.u32(person + person_profile_link))
        call(0x10308350, 0x103e6ab4, {c.u32(c.toc(-0x7b3c)), person, c.u32(row + 0x18), 0, 0xffffffff,
                                      std::uint32_t(std::int32_t(std::int8_t(signing))), 0});
    if (buyer) {
        const auto contract = call(find_contract, 0x103e6ac8, {contracts(), person});
        r[19] = contract;
        if (std::int32_t(fee) == -1) {
            const auto kind_of = [&] { return std::int8_t(std::int32_t(std::uint32_t(c.u8(contract + 0x38)) << 24 & 0xfe000000u) >> 26); };
            if (contract == 0 || kind_of() != 2) {
                r[25] = 0;
            } else {
                const auto length = std::int16_t(call(0x10374d10, 0x103e6af8, {}));
                // The contract's word +0xc times the staff contract length, divided by 7 (truncating).
                const auto product = std::int32_t(c.u32(contract + 0xc) * std::uint32_t(std::int32_t(length)));
                r[25] = std::uint32_t(product / 7);
            }
        }
        const auto account = call(0x10103510, 0x103e6b38, {c.u32(c.toc(-0x7ab8)), buyer});
        call(0x100f7170, 0x103e6b40, {account, r[25]});
        call(0x10365590, 0x103e6b58, {c.u32(manager + manager_shortlist), person, buyer, 4, 2});
        if (const auto club_link = c.u32(person + person_club_link)) {
            call(0x103afb90, 0x103e6b78, {c.u32(r[27]), club_link, person, 1, 1});
            call(0x103afb90, 0x103e6b90, {c.u32(r[27]), c.u32(person + person_club_link), person, 0, 1});
            call(0x100c8930, 0x103e6ba8, {c.u32(person + person_club_link), person, 0, 1, 0});
        }
        c.put16(person + 0x4e, c.u16(r[17]));
        c.put16(person + 0x4c, c.u16(today));
        c.put8(person + 0x50, c.u8(r[18]));
    }
    if (club_of_person(0x103e6bcc)) {
        const auto club = club_of_person(0x103e6c04);
        call(0x1007f840, 0x103e6c34, {contracts(), person, club});
    }
    call(0x10084a80, 0x103e6c40, {contracts(), person});
    c.put32(person + person_term, 0);
    c.put8(person + 0x6d, 0);
    call(0x103f29f0, 0x103e6c58, {manager, person});
    call(0x103f2aa0, 0x103e6c64, {manager, person});
    call(0x103f2b50, 0x103e6c70, {manager, person});
    call_native(0x103edef0, 0x103e6c80, {manager, person, 0});
    call(0x10006600, 0x103e6c90, {c.u32(c.toc(-0x7b24)), person});
}
}
