#pragma once
// The press's text generators over a story record (media.cpp, block text_mid).
#include "media.hpp"

namespace cm::aot::media {
void manager_outburst_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void staff_match_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void sent_off_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void outlet_report_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void signing_reaction_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void competition_win_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void player_sale_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void transfer_rumour_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
void transfer_rumour_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
}
