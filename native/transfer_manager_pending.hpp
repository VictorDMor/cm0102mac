#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {

// The cached marker is cleared if the contracts manager no longer has the
// pending transfer. On a hit, destination receives the club if requested.
void mark_pending_contract(guest::Cpu&, std::uint32_t manager, std::uint32_t person);
void clear_pending_contract(guest::Cpu&, std::uint32_t manager, std::uint32_t person);
std::uint32_t find_pending_contract(guest::Cpu&, std::uint32_t manager,
                                    std::uint32_t person, std::uint32_t destination);

}
