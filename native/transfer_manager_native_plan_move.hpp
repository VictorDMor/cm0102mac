#pragma once
// TRANSFER_MANAGER.CPP, block "plan": the state of one player's move to the buyer (0x103efec0), shared by its parts.
#include "guest_fpu.hpp"
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
struct PlayerMove {
    guest::Cpu& c;
    guest::Frame& frame;
    guest::Fpu fpu;
    std::uint32_t manager, person, club, offer;
    // Cells of the table of contents the original keeps in registers all along.
    std::uint32_t shortlists_cell, seed_cell, cursor_cell, pool, contracts_cell, found_cell, clubs_cell, news_cell;
    std::uint32_t date;                 // the game date (r31 points two bytes into it)
    std::uint32_t person_club = 0;      // the club of the person's contract (r15)
    std::uint32_t contract = 0;         // the person's contract record, or a stand-in built in the frame (r30)
    std::uint32_t person_kind = 0;      // the person's byte +0x62 at the start (r28)
    std::uint32_t free_move = 0, released = 0;  // r13 and r14 as the original sets them on the way
    std::uint32_t news_flag = 0;        // r23: a flag the contract-club lookup leaves, later other values
    std::uint32_t buyer_id = 0;         // the buyer club's id (frame +0x48)
    std::uint32_t seller_id = 0;        // frame +0x4c: the id of the club of the person's contract, or all ones

    PlayerMove(guest::Cpu& cpu, guest::Frame& f, std::uint32_t manager, std::uint32_t person, std::uint32_t club,
               std::uint32_t offer);

    // The club of the person's current contract, with the lookup's answer left in the shared cell. The
    // original clears register `flag_register` before the lookup and sets it to 1 when the contract is one with a club.
    std::uint32_t contract_club(std::uint32_t who, std::uint32_t back, unsigned flag_register = 23);
    std::uint32_t people() const { return c.u32(c.toc(toc::people)); }
    std::uint32_t club_at(std::uint32_t id) const { return c.u32(clubs_cell) + id * club_size; }
    std::uint32_t contracts() const { return c.u32(contracts_cell); }
    std::uint32_t manager_table() const { return c.u32(manager + 0x988); }
    // The halfword generator at its two cells: advances the cursor (calling the refill at `back` when it wraps).
    void draw_advance(std::uint32_t back);
    std::uint32_t draw_value() const { return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(seed_cell); }
    // The same lookup without the contract-kind test (and without the flag register).
    std::uint32_t found_club_or_zero(std::uint32_t who, std::uint32_t back);
    std::uint32_t cell_value(std::int32_t toc_offset) const { return c.u32(c.toc(toc_offset)); }
    std::uint32_t person_at(std::uint32_t id) const { return people() + id * person_size; }
    void note_released_person_a();  // 0x103f07b0 .. 0x103f0a68, the person leaves through a contract that ended
    void zero_contract_path();      // 0x103f09a0 ..

    void snapshot();               // 0x103efec0 .. 0x103f0388
    void file_and_clear_offers();  // 0x103f0388 .. 0x103f05bc
    void end_contract();           // 0x103f05bc .. 0x103f0a68
    void leave_old_club();         // 0x103f0a68 .. 0x103f0e60
    void adjust_values();          // 0x103f0e60 .. 0x103f18f0
    void finish();                 // 0x103f18f0 .. 0x103f1b30
};
}
