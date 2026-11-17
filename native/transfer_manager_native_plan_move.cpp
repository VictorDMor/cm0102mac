// TRANSFER_MANAGER.CPP, block "plan": one player moves to the buyer (0x103efec0), first part: the
// entry, what is noted about the move, the old club's shortlist rows and the club's lists.
#include "transfer_manager_native_plan.hpp"
#include "transfer_manager_native_plan_move.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20;
constexpr std::int32_t kind_five = 5;
// Frame slots of 0x103efec0 (offsets from its stack pointer).
constexpr std::uint32_t slot_move = 0x38, slot_date = 0x58, slot_stand_in = 0x60, slot_buyer_id = 0x48, slot_seller_id = 0x4c,
                        slot_fee = 0x50, slot_route = 0x54, slot_free_flag = 0x118, slot_news_made = 0x119,
                        slot_negotiation = 0x11c;
}

PlayerMove::PlayerMove(Cpu& cpu, Frame& f, std::uint32_t manager_, std::uint32_t person_, std::uint32_t club_, std::uint32_t offer_)
    : c(cpu), frame(f), fpu(cpu), manager(manager_), person(person_), club(club_), offer(offer_),
      shortlists_cell(cpu.toc(-0x7b9c)), seed_cell(cpu.toc(-0x7b90)), cursor_cell(cpu.toc(-0x7b84)), pool(cpu.toc(-0x4ba0)),
      contracts_cell(cpu.toc(toc::contracts)), found_cell(cpu.toc(toc::found_contract)), clubs_cell(cpu.toc(toc::clubs)),
      news_cell(cpu.toc(-0x79f0)), date(cpu.toc(-0x7b80)) {
    keep(c, 24, manager);
    keep(c, 25, person);
    keep(c, 26, club);
    keep(c, 27, offer);
    keep(c, 16, shortlists_cell);
    keep(c, 23, news_cell);
    keep(c, 17, seed_cell);
    keep(c, 18, cursor_cell);
    keep(c, 19, pool);
    keep(c, 20, contracts_cell);
    keep(c, 21, found_cell);
    keep(c, 22, clubs_cell);
}

std::uint32_t PlayerMove::found_club_or_zero(std::uint32_t who, std::uint32_t back) {
    const auto found = invoke(c, find_contract, back, {contracts(), who});
    c.put32(found_cell, found);
    return found ? club_at(c.u32(found + 4)) : 0;
}

std::uint32_t PlayerMove::contract_club(std::uint32_t who, std::uint32_t back, unsigned flag_register) {
    keep(c, flag_register, 0);
    const auto found = invoke(c, find_contract, back, {contracts(), who});
    c.put32(found_cell, found);
    const bool with_club = found != 0 && (c.i8(found + 0x38) >> 2) != kind_five;
    keep(c, flag_register, with_club);
    return with_club ? club_at(c.u32(c.u32(found_cell) + 4)) : 0;
}

void PlayerMove::draw_advance(std::uint32_t back) {
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    if (next > c.u32(c.toc(-0x7b88))) {
        c.put32(cursor_cell, c.toc(-0x7b8c));
        const auto fresh = invoke(c, 0x1077b770, back);
        c.put32(seed_cell, fresh & 0xffff);
    }
}

void move_player_to_buyer(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t offer) {
    Frame frame(c, 0x103efec0, 0x180, {.first_gpr = 13});
    PlayerMove move(c, frame, manager, person, club, offer);
    move.snapshot();
    move.file_and_clear_offers();
    move.end_contract();
    move.leave_old_club();
    move.adjust_values();
    move.finish();
}

void PlayerMove::snapshot() {
    invoke(c, 0x103721f0, 0x103eff08, {frame.at(slot_stand_in)});
    invoke(c, 0x10715880, 0x103eff10, {frame.at(0xc8)});
    c.put8(frame.at(0x120), 0);
    c.put8(frame.at(slot_news_made), 0);
    keep(c, 13, 0);
    keep(c, 14, 0);
    const auto scouts = cell_value(-0x7440);
    if (scouts != 0 && club != 0 && c.u32(club + 0x90) != 0) invoke(c, 0x10700640, 0x103eff54, {scouts, c.u32(club + 0x90), person});
    person_kind = c.u8(person + 0x62);
    keep(c, 28, person_kind);
    person_club = found_club_or_zero(person, 0x103eff64);
    keep(c, 15, person_club);
    c.put8(frame.at(slot_free_flag), 0);
    const auto terms = invoke(c, 0x10082f90, 0x103effa0, {contracts(), person, person_club});
    if (terms != 0 && c.i8(terms + 0x44) > 0 && c.i8(terms + 0x45) < 5) c.put8(frame.at(slot_free_flag), 1);
    contract = invoke(c, 0x1007f720, 0x103effdc, {contracts(), c.u32(person), c.u32(club)});
    keep(c, 30, contract);
    if (contract == 0) {
        // No contract with the club: a stand-in is built in the frame.
        invoke(c, 0x10374f50, 0x103f0008, {frame.at(slot_stand_in), person, club, 0xffffffff, 0xffffffff, 0xb, 0, 0});
        c.put8(frame.at(0xd8), 0xb);
        contract = frame.at(slot_stand_in);
        keep(c, 30, contract);
        c.put32(frame.at(0xdc), 0xffffffff);
    }
    if (c.u8(person + 0x60) == 5 && c.u32(person + 0x20) != 0 && c.u8(offer + 0x11) != 9)
        invoke(c, 0x101aa680, 0x103f0054, {c.u32(news_cell), c.u32(person + 0x20), 4, 0, 0});
    const auto role = c.i8(person + 0x62);
    if ((role == 5 || role == 0xc) && c.u32(person + 0x24) != 0)
        invoke(c, 0x101aa680, 0x103f008c, {c.u32(news_cell), c.u32(person + 0x24), 4, 0, 0});
    invoke(c, 0x10715880, 0x103f0094, {frame.at(slot_date)});
    buyer_id = c.u32(club);
    c.put32(frame.at(slot_buyer_id), buyer_id);
    // The club of the person's contract, asked again for its id (all ones without one).
    std::uint32_t seller = 0xffffffff;
    if (contract_club(person, 0x103f00ac) != 0) {
        const auto second = contract_club(person, 0x103f0110);
        seller = c.u32(second);
    }
    seller_id = seller;
    c.put32(frame.at(slot_seller_id), seller);
    // The date, and the record of the move handed on to 0x103f2d50 below.
    keep(c, 31, date + 2);
    c.put16(frame.at(slot_date + 2), c.u16(date + 2));
    c.put16(frame.at(slot_date), c.u16(date));
    c.put8(frame.at(slot_date + 4), c.u8(date + 4));
    c.put32(frame.at(slot_move), c.u32(person));
    c.put32(frame.at(slot_move + 4), c.u32(c.u32(person + 0x28) + 0x20));
    c.put32(frame.at(slot_move + 8), c.u32(c.u32(person + 0x2c) + 0x20));
    c.put32(frame.at(slot_move + 12), c.u32(c.u32(person + 0x30) + 0x20));
    // How the offer reached the person (the move's route byte at +0x54).
    const auto kind = std::int32_t(c.i8(offer + 0x38) >> 4);
    if (c.i8(offer + 0x37) != 0 && c.u32(offer + 0x20) == 0 && (kind == 3 || kind == 4)) {
        free_move = 1;
        keep(c, 13, free_move);
        c.put8(frame.at(slot_route), 4);
    } else {
        c.put8(frame.at(slot_route), c.u32(offer + 4) == 0 ? 0 : 1);
    }
    if (std::int8_t(invoke(c, 0x100c7a20, 0x103f0228, {c.u8(offer + 0x11)})) == 0) c.put8(frame.at(slot_route), 5);
    c.put32(frame.at(slot_fee), c.u32(person) == c.u32(offer + 0xc) ? c.u32(offer + 0x20) : 0);
    // A big fee for a club whose owner would not have paid it: two draws and the owner's reaction.
    const auto owner = c.u32(club + 0x80);
    const auto fee = std::int32_t(c.u32(offer + 0x20));
    if (owner != 0 && c.u32(owner + 0x1c) != 0 && fee > 2500000) {
        const auto profile = c.u32(owner + 0x1c);
        const std::uint32_t limit = std::uint32_t(std::int32_t(c.i16(club + 0x1ec))) *
                                    std::uint32_t(std::int32_t(c.i8(owner + 0x63)) + std::int32_t(c.i8(profile + 0x40)) + 0x2ee);
        if (std::int32_t(limit) < fee) {
            draw_advance(0x103f02d8);
            const auto first = std::int32_t(draw_value());
            const auto remainder = first - first / 30 * 30;
            const auto temper = std::int32_t(c.i8(c.u32(c.u32(club + 0x80) + 0x1c) + 0x36));
            if (temper > remainder) {
                draw_advance(0x103f0350);
                const auto second = draw_value();
                const auto sign = second >> 31;
                const auto signed_parity = std::int8_t(std::int32_t((second & 1) ^ sign) - std::int32_t(sign));
                invoke(c, 0x100d9960, 0x103f0388, {c.u32(club + 0x80), person, std::uint32_t(std::int32_t(signed_parity))});
            }
        }
    }
}

void PlayerMove::file_and_clear_offers() {
    invoke(c, 0x103f2d50, 0x103f0394, {manager, frame.at(slot_move)});
    // The club's table of its 25 open items: the one that names this person (kind 0xb) is settled.
    const auto row_base = c.u32(manager_table() + 0xdc);
    const auto row = row_base + c.u32(club) * 0xc;
    c.set(8, row_base);  // the original leaves the base in r8; 0x10362d70 and 0x10365590 read r8 at 0x103f0558/0x103f05b8
    news_flag = row;
    keep(c, 23, row);
    const auto all_done = std::int8_t(invoke(c, 0x1014c2e0, 0x103f03c0, {club, 0, 1, 0, 0}));
    if (all_done == 0 && row != 0) {
        std::uint32_t offset = 0;
        for (std::int8_t index = 0; index < 0x19; ++index, offset += 0x2c) {
            const auto entry = c.u32(row) + offset;
            keep(c, 29, entry);
            if (c.u32(entry + 0x10) != c.u32(person) || c.u8(entry) != 0xb) continue;
            const auto rating = invoke(c, 0x10216830, 0x103f0420, {person, 0xffffffff, 0x5e82, club, 0xffffffff, 0});
            c.put32(frame.at(slot_negotiation), rating);
            c.put8(frame.at(slot_news_made), 1);
            const auto linked = std::int32_t(c.u32(entry + 0x14));
            if (linked <= -1) break;
            const auto other = person_at(std::uint32_t(linked));
            if (contract_club(other, 0x103f0454) != club) break;
            const auto terms = invoke(c, 0x10082f90, 0x103f04c8, {contracts(), person_at(c.u32(entry + 0x14)), club});
            // The terms' mood byte drops by 50, down to -100 (the original's two clamps at 0x103f04e0-0x103f0508 never fire).
            const auto mood = std::int16_t(std::int32_t(c.i8(terms + 0x44)) - 0x32);
            c.put8(terms + 0x44, std::uint8_t(mood < -100 ? -100 : mood));
            invoke(c, 0x10363ca0, 0x103f0534, {manager_table(), person_at(c.u32(entry + 0x14)), 0});
            break;
        }
    }
    if (std::int8_t(invoke(c, 0x10362d70, 0x103f055c, {manager_table(), person, club})) != 0) {
        const auto kind = std::int32_t(std::int8_t(c.u8(offer + 0x11)));
        invoke(c, 0x10365590, 0x103f0594, {manager_table(), person, club, 2, kind != 11 ? 1u : 0u, std::uint32_t(kind)});
    }
    const auto previous_club = c.u32(person + 0x24);
    if (previous_club != 0 && previous_club != club) invoke(c, 0x10365590, 0x103f05bc, {manager_table(), person, previous_club, 4, 2});
    // The contract's club, up to three times (each lookup leaves its answer in the shared cell).
    if (c.u32(person + 0x24) != contract_club(person, 0x103f05cc)) {
        if (invoke(c, 0x10083110, 0x103f0630, {contracts(), person}) != 0) {
            if (contract_club(person, 0x103f0648) != 0) {
                const auto third = contract_club(person, 0x103f06ac);
                invoke(c, 0x10365590, 0x103f070c, {manager_table(), person, third, 4, 2});
            }
        }
    }
}
}
