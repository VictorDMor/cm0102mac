// TRANSFER_MANAGER.CPP native-first block "approaches": offers made for players the game creates or signs
// (0x103e3410, 0x103e3de0).
#include "transfer_manager_native_approaches.hpp"
#include "transfer_manager_native_approaches_detail.hpp"
#include "transfer_manager_native_status.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
using namespace approaches;
namespace {
// A date is a halfword (+0), a halfword (+2) and a byte (+4). The original orders two dates by +2 as unsigned
// halfwords and, when equal, by +0 through an xor/shift form: bit 31 of ((x >> 1) - (x & a)), x = a ^ b,
// both signed halfwords.
bool first_halfword_before(std::int16_t a, std::int16_t b) {
    const std::int32_t ea = a, eb = b, x = ea ^ eb;
    return ((std::uint32_t(x >> 1) - std::uint32_t(x & ea)) >> 31) != 0;
}
}

// ---- 0x103e3410 ----
void add_regen_offer(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t date,
                     std::uint32_t amount, std::int32_t cap, std::uint32_t years) {
    constexpr std::uint32_t local_offer = 0xfc, terms = 0x98, terms_cap = 0xa4, terms_amount = 0xbc, terms_start = 0xc0,
                            terms_end = 0xc8, new_date = 0x40, next_date = 0x38;
    constexpr std::uint32_t find_contract = 0x10082f20;
    Frame frame(c, 0x103e3410, 0x1b0, {.first_gpr = 16});
    const HalfwordDraw draw(c);
    const auto contracts_cell = c.toc(toc::contracts), found_cell = c.toc(-0x7b78), clubs_cell = c.toc(-0x7be8),
               flag_address = c.toc(-0x7bac), buffer = c.toc(-0x7ba8), pool2 = c.toc(-0x4b9c);
    keep(c, 16, manager);
    keep(c, 17, person);
    keep(c, 18, club);
    keep(c, 19, date);
    keep(c, 20, amount);
    keep(c, 21, unsigned_of(cap));
    keep(c, 22, years);
    keep(c, 23, draw.seed);
    keep(c, 24, draw.cursor);
    keep(c, 25, contracts_cell);
    keep(c, 26, found_cell);
    keep(c, 27, clubs_cell);
    keep(c, 28, flag_address);
    keep(c, 29, buffer);
    keep(c, 30, pool2);
    invoke(c, 0x10411e60, 0x103e3464, {frame.at(local_offer)});
    invoke(c, 0x103721f0, 0x103e346c, {frame.at(terms)});
    if (amount == 0xffffffff) {
        amount = c.u32(person + 8);
        keep(c, 20, amount);
    }
    const auto contract = invoke(c, find_contract, 0x103e3484, {c.u32(contracts_cell), person});
    keep(c, 31, contract);
    if (invoke(c, 0x103eaea0, 0x103e349c, {manager, person, 0, 0}) != 0) {
        // The person already has an offer of this kind: report it twice (the second after 0x100caeb0).
        report_assertion(c, {0x103e34b8, pool2 + 0x37, pool2 + 0x6ff, pool2 + 0x70c, frame.at(0x78), frame.at(0x50), 0x6ce});
        invoke(c, 0x100caeb0, 0x103e3508, {person, c.toc(-0x7bb8), 0x7d0, 1});
        report_assertion(c, {0x103e351c, pool2 + 0x37, pool2 + 0x6ff, pool2 + 0x70c, frame.at(0x58), frame.at(0x48), 0x6d0});
        invoke(c, 0x10411f10, 0x103e3564, {frame.at(local_offer), 0xffffffff});
        return;
    }
    invoke(c, 0x104126d0, 0x103e3590, {frame.at(local_offer), person, club, 0, amount, amount == 0 ? 1u : 0u, 0});
    c.put8(frame.at(local_offer + 0x38), (c.u8(frame.at(local_offer + 0x38)) & 0x0f) | 0x30);
    c.put32(frame.at(local_offer + 0x20), amount);
    c.put8(frame.at(local_offer + 0x11), 0xb);
    invoke_native(c, 0x103e25d0, 0x103e35bc, {manager, frame.at(local_offer), 0});
    invoke(c, 0x10374f50, 0x103e35e0, {frame.at(terms), person, club, c.u32(frame.at(local_offer + offer::slot)), 2, 0xb, 0, 0});
    if (cap != -1) {
        c.put32(frame.at(terms_cap), unsigned_of(cap));
    } else {
        const auto account = invoke(c, 0x10103510, 0x103e3600, {c.u32(c.toc(-0x7ab8)), club});
        const auto worth = invoke(c, 0x100f2680, 0x103e360c, {account, person, 0xffffffff});
        if (signed_of(c.u32(frame.at(terms_cap))) > signed_of(worth)) c.put32(frame.at(terms_cap), worth);
    }
    if (c.u32(frame.at(terms_amount)) != 0) {
        // The contract's amount: the larger of amount scaled and amount + 250000, through the original's
        // int-to-double pairs.
        guest::Fpu fpu(c);
        const auto pool = c.toc(-0x4ba0);
        const auto magic = c.f64(pool + 8);
        const auto as_double = [](std::uint32_t word) { return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | word); };
        auto scaled = fpu.fmul(c.f64(pool), fpu.fsub(as_double(amount ^ 0x80000000u), magic));
        const auto raised_word = (amount + 250000) ^ 0x80000000u;
        const auto raised = fpu.fsub(as_double(raised_word), magic);
        fpu.fcmp(0, raised, scaled);
        if ((c.cr(0) & 4) != 0) scaled = fpu.fsub(as_double(raised_word), magic);
        c.put32(frame.at(terms_amount), std::uint32_t(fpu.fctiwz(0, scaled)));
    }
    if (std::int8_t(years) < 1) {
        draw.advance(0x103e36c8);
        years = byte_signed(std::uint8_t(signed_of(draw.value()) % 4 + 2));
        keep(c, 22, years);
    }
    c.put16(frame.at(terms_start + 2), c.u16(date + 2));
    c.put16(frame.at(terms_start), c.u16(date));
    c.put8(frame.at(terms_start + 4), c.u8(date + 4));
    invoke(c, 0x10717890, 0x103e372c,
           {frame.at(new_date), date, unsigned_of(std::int32_t(std::int16_t(std::int32_t(std::int8_t(years)) * 0x16d)))});
    c.put16(frame.at(terms_end + 2), c.u16(frame.at(new_date + 2)));
    c.put16(frame.at(terms_end), c.u16(frame.at(new_date)));
    c.put8(frame.at(terms_end + 4), c.u8(frame.at(new_date + 4)));
    const auto slot = c.u32(frame.at(local_offer + offer::slot));
    invoke(c, 0x1007f200, 0x103e3768,
           {c.u32(contracts_cell), frame.at(terms), slot, c.u32(frame.at(local_offer + offer::serial)), 0xb, 0, 0, 1});
    invoke(c, 0x103f2ec0, 0x103e3784, {manager, c.u32(c.u32(manager) + c.u32(frame.at(local_offer + offer::slot)) * 4), date});
    if (amount == 0) {
        const auto first = invoke(c, find_contract, 0x103e3798, {c.u32(contracts_cell), person});
        c.put32(found_cell, first);
        const auto owner = first != 0 ? club_of(c, clubs_cell, first) : 0;
        if (owner != 0) {
            const auto second = invoke(c, find_contract, 0x103e37d0, {c.u32(contracts_cell), person});
            c.put32(found_cell, second);
            const auto second_owner = second != 0 ? club_of(c, clubs_cell, second) : 0;
            if (std::int8_t(invoke(c, 0x100d33d0, 0x103e37fc, {c.u32(second_owner + 0x2c)})) != 0) {
                c.put8(contract + 0x54, 1);
                c.put8(contract + 0x5c, c.u8(contract + 0x5c) & 0xf7);
                invoke(c, 0x10717b20, 0x103e3828, {frame.at(next_date), date, 1});
                c.put16(contract + 0x32, c.u16(frame.at(next_date + 2)));
                c.put16(contract + 0x30, c.u16(frame.at(next_date)));
                c.put8(contract + 0x34, c.u8(frame.at(next_date + 4)));
            }
        }
    }
    invoke(c, 0x10411f10, 0x103e384c, {frame.at(local_offer), 0xffffffff});
}

// ---- 0x103e3de0 ----
void sign_person(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t from_date,
                 std::uint32_t to_date) {
    constexpr std::uint32_t terms = 0x5c, level_flag = 0x38, moved_date = 0x54, terms_date = 0x84;
    Frame frame(c, 0x103e3de0, 0xf0, {.first_gpr = 23});
    const auto squad_cell = c.toc(-0x7a98), today_cell = c.toc(-0x7b80), contracts_cell = c.toc(-0x7b7c);
    keep(c, 29, squad_cell);
    keep(c, 23, manager);
    keep(c, 30, today_cell);
    keep(c, 24, person);
    keep(c, 31, contracts_cell);
    keep(c, 25, club);
    keep(c, 26, from_date);
    keep(c, 27, to_date);
    const auto contracts = [&] { return c.u32(contracts_cell); };
    invoke(c, 0x103721f0, 0x103e3e18, {frame.at(terms)});
    auto loan_club = invoke(c, 0x100ce320, 0x103e3e28, {club, frame.at(level_flag), 1});
    keep(c, 28, loan_club);
    if (loan_club != 0 && std::int8_t(c.u8(frame.at(level_flag))) != 0) {
        loan_club = 0;
        keep(c, 28, 0);
    }
    const auto level = invoke(c, 0x100c9070, 0x103e3e50, {club, 0, 0});
    if (std::int8_t(level) >= 0x32) return;
    if (invoke(c, 0x10083110, 0x103e3e68, {contracts(), person}) != 0) return;

    const auto today = today_cell;  // r30 holds the pointer to the game date
    const std::uint32_t today_day = c.u16(today + 2);
    const std::int16_t today_year = std::int16_t(c.u16(today));
    // The start date must not be past today, and the end date must not be before it.
    const std::uint32_t from_day = c.u16(from_date + 2);
    const bool from_past = from_day > today_day || (from_day == today_day && first_halfword_before(std::int16_t(c.u16(from_date)), today_year));
    if (from_past) return;
    const std::uint32_t to_day = c.u16(to_date + 2);
    const bool to_past = to_day < today_day || (to_day == today_day && first_halfword_before(today_year, std::int16_t(c.u16(to_date))));
    if (to_past) return;
    const auto contract = invoke(c, 0x10082f20, 0x103e3f34, {contracts(), person});
    if (contract == 0) return;

    // The person's recorded date (+0x54) moves up to the end date when it is earlier.
    const std::uint32_t recorded_day = c.u16(person + 0x56);
    const bool recorded_before = recorded_day < to_day || (recorded_day == to_day && first_halfword_before(std::int16_t(c.u16(to_date)), std::int16_t(c.u16(person + 0x54))));
    if (recorded_before) {
        c.put16(person + 0x56, to_day);
        c.put16(person + 0x54, c.u16(to_date));
        c.put8(person + 0x58, c.u8(to_date + 4));
        const auto again = invoke(c, 0x10082f20, 0x103e3fb8, {contracts(), person});
        c.put16(again + 0x32, c.u16(to_date + 2));
        c.put16(again + 0x30, c.u16(to_date));
        c.put8(again + 0x34, c.u8(to_date + 4));
    }
    const auto signing_club = loan_club != 0 ? loan_club : club;
    invoke(c, 0x103723b0, 0x103e4000, {frame.at(terms), person, signing_club, to_date, 0, 1, 1});
    c.put16(frame.at(terms_date + 2), c.u16(from_date + 2));
    c.put16(frame.at(terms_date), c.u16(from_date));
    c.put8(frame.at(terms_date + 4), c.u8(from_date + 4));
    invoke(c, 0x10083e50, 0x103e4024, {contracts(), frame.at(terms)});
    invoke(c, 0x100df500, 0x103e4034, {c.u32(c.toc(-0x7aa4)), person});
    invoke(c, 0x100ce200, 0x103e4040, {person, 1});
    if (c.u8(person + 0x6d) == 2) c.put8(person + 0x6d, 0x10);
    if (std::int8_t(invoke(c, 0x104ae570, 0x103e4074, {c.u32(squad_cell), signing_club, 0})) != 0)
        invoke(c, 0x104ad840, 0x103e409c, {c.u32(squad_cell), signing_club, 0});
    if (loan_club != 0 && loan_club != club) invoke(c, 0x1007ee50, 0x103e40bc, {contracts(), person, loan_club});
    if (std::int8_t(invoke(c, 0x103ecef0, 0x103e40d0, {manager, person, club, 0})) != 0) {
        invoke(c, 0x10717890, 0x103e40e8, {frame.at(moved_date), to_date, 7});
        c.put16(to_date + 2, c.u16(frame.at(moved_date + 2)));
        c.put16(to_date, c.u16(frame.at(moved_date)));
        c.put8(to_date + 4, c.u8(frame.at(moved_date + 4)));
        invoke(c, 0x103ec940, 0x103e411c, {manager, person, c.u32(club + 0x2c), c.u16(from_date + 2), 1, 1});
    }
}
}
