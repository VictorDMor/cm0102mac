#pragma once
// TRANSFER_MANAGER.CPP native-first block "plan": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x103efaa0: an offer has been agreed; closes it for the player and every listed player, and moves the contracts on.
void conclude_agreed_offer(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x103efec0: one player of an agreed offer changes club (contract, shortlist, squad, finances, news).
void move_player_to_buyer(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club, std::uint32_t offer);
// 0x103edef0: updates (or drops) the manager's row for the person whose index `person_index` points to (`club`: the new club, may be 0).
void update_person_row(guest::Cpu& c, std::uint32_t manager, std::uint32_t person_index, std::uint32_t club);
// 0x103f2c00: finds the first person with the given role at the club and files it with 0x100c82a0 (`kind` is the raw r5).
void pass_on_person_of_kind(guest::Cpu& c, std::uint32_t unused, std::uint32_t club, std::uint32_t kind);
}
