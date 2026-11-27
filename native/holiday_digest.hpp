#pragma once
#include "guest_abi.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot::holiday_digest {
// The readers pass a current guest club address. Only stable IDs and game-encoded text travel in saves.
bool tracking(const State&, std::uint32_t club);
void record_event(State&, std::uint32_t club, std::string_view section, std::string line);
void transition(guest::Cpu&, std::uint32_t manager, bool away);
std::vector<std::string> article_lines(const State&, std::uint32_t report_id);
}
