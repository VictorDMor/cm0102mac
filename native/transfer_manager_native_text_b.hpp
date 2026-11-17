#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_b": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// 0x104001e0: text of news message 4001 into the output (item = the news item, output = the message being built).
void news_text_4001(guest::Cpu& c, std::uint32_t item, std::int8_t detail, std::uint32_t output);
// 0x10401c90: text of news message 4002.
void news_text_4002(guest::Cpu& c, std::uint32_t item, std::uint32_t detail, std::uint32_t output);
// 0x104026b0: text of news message 4003.
void news_text_4003(guest::Cpu& c, std::uint32_t item, std::int8_t detail, std::uint32_t output);
}
