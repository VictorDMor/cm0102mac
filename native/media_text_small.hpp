#pragma once
// The shorter press texts: headline and article formatters over a news item. Each takes the
// item (values read through 0x102cb570), a flag choosing the long form (`full`, a signed byte)
// and the output text, and builds the strings in its stack buffers.
#include "media.hpp"

namespace cm::aot::media {
void club_finance_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
void player_apology_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
void manager_exchange_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
void manager_reply_text(guest::Cpu&, std::uint32_t self, std::int32_t kind, std::int32_t tone, std::uint32_t output, std::int32_t level);
void player_request_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
void player_quarrel_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
void replay_logged_item(guest::Cpu&, std::uint32_t date, std::uint32_t id);
void media_response_dialog(guest::Cpu&);
void media_response_event(guest::Cpu&, std::uint32_t control);
void player_outburst_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
}
