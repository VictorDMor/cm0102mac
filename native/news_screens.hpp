#pragma once
#include "guest_abi.hpp"

// NEWS_SCREENS.CPP: the manager's news screen and the send-message dialog.
namespace cm::aot::news_screens {
using guest::Cpu;
inline constexpr int read_all_action = 30510;
inline constexpr unsigned reload_select_newest = 2; // value 1 is the usual list refresh
// The news screen, the screen manager's callbacks for it.
void open_news(Cpu&, std::uint32_t person, std::uint32_t check_attention);
void draw_news(Cpu&);
std::uint32_t news_command(Cpu&, std::uint32_t control);
void close_news(Cpu&);
// The send-message dialog.
void open_message(Cpu&, std::uint32_t recipient, std::uint32_t reply_to);
void open_reply(Cpu&, std::uint32_t manager, std::uint32_t item);
void draw_message(Cpu&);
std::uint32_t message_command(Cpu&, std::uint32_t control);
}
