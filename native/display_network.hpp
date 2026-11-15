#pragma once
// DISPLAY.CPP block "network": see the block's .cpp for what it does.
#include "guest_abi.hpp"

namespace cm::aot::display {
// part 1: sending the display table over the network (display_network_send.cpp).
// 0x1071c2d0: sends one part of the screen to the peer (r6 = 0 header, 1 GUI objects as a difference
// against the copy of the last send, 2 the stream built by the commands, 3 the areas) through the
// global stream (TOC -0x5c04); returns the byte count, or 0 when the send fails.
std::uint32_t send_screen_part(guest::Cpu&, std::uint32_t table, std::uint32_t peer, std::uint32_t context,
                               std::uint32_t part);
// 0x1071c1d0: sends the whole screen (header, then the other three parts unless the table is
// marked) and logs the byte counts.
void send_screen(guest::Cpu&, std::uint32_t table, std::uint32_t peer, std::uint32_t context);
// 0x10723650: rewrites a resource path as the game's own folder (pictures, sounds or data) names it.
void localize_resource_path(guest::Cpu&, std::uint32_t path, std::uint32_t pictures_first);
// 0x10723850: sends the two-byte reply {5, byte}.
void send_reply(guest::Cpu&, std::uint32_t peer, std::uint32_t context, std::uint32_t byte);
}
// part 2
namespace cm::aot::display {
void send_ready(guest::Cpu& c, std::uint32_t table, std::uint32_t destination);                                  // 0x10723a30
bool send_gui_object(guest::Cpu& c, std::uint32_t table, std::uint32_t destination, std::uint32_t index_word);  // 0x10723cb0
}
