#pragma once

#include "guest_abi.hpp"

namespace cm::aot::transfer_manager {
void resolve_squad_quota_category(guest::Cpu&, std::uint32_t competition, std::uint32_t kind,
                                  std::uint32_t nation, std::uint32_t group);
std::uint32_t team_squad_quota_status(guest::Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t competition);
void read_squad_quota_limits(guest::Cpu&, std::uint32_t manager, std::uint32_t competition, std::uint32_t limits,
                             std::uint32_t reduce);
void select_squad_quota_limit(guest::Cpu&, std::uint32_t manager, std::uint32_t competition, std::uint32_t limit);
std::uint32_t squad_violates_quota(guest::Cpu&, std::uint32_t manager, std::uint32_t club, std::uint32_t competition,
                                   std::uint32_t message, std::uint32_t lineup, std::uint32_t counts);
}
