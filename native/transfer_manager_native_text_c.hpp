#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_c": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// The text of news message 4012 (0x10402e70): a move between two clubs, worded by 
void news_text_4012(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4014 (0x10403460): a person offered to a club; names the refusal reasons of 0x10410e50 when there are any.
void news_text_4014(guest::Cpu& c, std::uint32_t owner, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4015 (0x10403a90): a deal between two clubs on two dates, with an amount as cash text when it is the larger.
void news_text_4015(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4007 (0x10406af0): three clubs and an amount as cash text.
void news_text_4007(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4006 (0x10406fd0): a club and an amount as cash text, in four wordings.
void news_text_4006(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 4008 (0x10407a50): a deal for a person in about forty wordings.
void news_text_4008(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
}
